/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "jack_interface.hpp"

#include <libril/constants.hpp>
#include <libril/detail/compose_message_string.hpp>

#include <librbbl/index_sequence.hpp>

#include <jack/jack.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <cassert>
#include <ciso646> // should not be necessary in C++11, but MSVC is non-compliant here
#include <iostream>

#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace visr
{
  namespace audiointerfaces
  {

    namespace // unnamed
    {
      // Lookup table to translate error enums to messages.
      using JackStatusDesc = std::tuple<jack_status_t, char const *>;
      static std::vector< JackStatusDesc > const cJackStatusTranslator = {
          JackStatusDesc{ JackFailure, "Overall operation failed" },
          JackStatusDesc{ JackInvalidOption, "Invalid or unsupported option" },
          JackStatusDesc{ JackNameNotUnique, "Client name not unique" },
          JackStatusDesc{ JackServerStarted, "JACK server was started" },
          JackStatusDesc{ JackServerFailed, "Unable to connect to the JACK server" },
          JackStatusDesc{ JackServerError, "Communication error with the JACK server" },
          JackStatusDesc{ JackNoSuchClient, "Requested client does not exist" },
          JackStatusDesc{ JackLoadFailure, "Unable to load internal client" },
          JackStatusDesc{ JackInitFailure, "Unable to initialize client" },
          JackStatusDesc{ JackShmFailure, "Unable to access shared memory" },
          JackStatusDesc{ JackVersionError, "Client's protocol version does not match" }
      };
    } // unnamed namespace

    /******************************************************************************/
    /* Definition of the internal implementation class JackInterface::Impl   */
    
    class JackInterface::Impl
    {
    public:
      explicit Impl( Configuration const & baseConfig, std::string const & config );
      
      ~Impl( );
      
      
      void start();
      
      void stop();
      
      bool registerCallback( AudioCallback callback, void* userData );
      
      bool unregisterCallback( AudioCallback audioCallback );
      
    private:
      
      JackInterface::Config parseSpecificConf( std::string const & config );
      
      void process( jack_nframes_t numFrames );
      
      int bufferSizeChange( jack_nframes_t newBufferSize );
      
      int sampleRateChange( jack_nframes_t newSampleRate );
      
      void serverShutdown();
      
      int underrun();
      
      static int processCallbackFunction( jack_nframes_t numFrames, void* arg );
      
      static int bufferSizeCallbackFunction( jack_nframes_t nframes, void *arg );
      
      static int sampleRateCallbackFunction(jack_nframes_t nframes, void *arg);
      
      static void shutdownCallbackFunction( void *arg );
      
      static int underrunCallbackFunction( void *arg );
      
      void registerPorts();
      
      void unregisterPorts();
      
      void connectPorts();
      
      void disconnectPorts();
      
      void setCaptureBuffers( jack_nframes_t numFrames );
      
      void setPlaybackBuffers( jack_nframes_t numFrames );
      
      
      
      std::size_t const mNumCaptureChannels;
      std::size_t const mNumPlaybackChannels;
      
      
      std::size_t const mPeriodSize;
      std::size_t mSampleRate;
      
      
      
      Base::AudioCallback mCallback;
      
      void* mCallbackUserData;
      
      /**
       * Buffer to hold the pointers to the sample vectors for the input
       * of the audio processor.
       * These samples are written from the capture ports of the sound
       * interface and then passed to the audio processing callback function.
       */
      std::vector< SampleType const * > mCaptureSampleBuffers;
      
      /**
       * Buffer to hold the pointers to the sample vectors for the output
       * of the audio processor.
       * These samples are generated by the audio processing callback
       * function and then passed to the playback argument of the
       * portaudio callback function.
       */
      std::vector< SampleType * > mPlaybackSampleBuffers;
      
      jack_client_t* mClient;
      
      
      bool mInitialised;
      
      
      std::string mClientName;
      std::vector<std::string>  mCapturePortNames;
      std::vector<std::string>  mPlaybackPortNames;
      std::vector<std::string>  mInExternalPortNames;
      std::vector<std::string>  mOutExternalPortNames;
      
      std::string mInExtClientName;
      std::string mOutExtClientName;
      bool mInAutoConnect;
      bool mOutAutoConnect;
      
      std::vector<jack_port_t*> mCapturePorts;
      
      std::vector<jack_port_t*> mPlaybackPorts;
      
    };
    
    /******************************************************************************/
    /* Implementation of the JackInterface::Impl class                       */
    
    JackInterface::Impl::Impl( Configuration const & baseConfig, std::string const & conf )
    : mNumCaptureChannels( baseConfig.numCaptureChannels() )
    , mNumPlaybackChannels( baseConfig.numPlaybackChannels() )
    , mPeriodSize( baseConfig.periodSize() )
    , mSampleRate( baseConfig.sampleRate() )
    , mCallback( nullptr )
    , mCallbackUserData( nullptr )
    , mCaptureSampleBuffers( mNumCaptureChannels, nullptr )
    , mPlaybackSampleBuffers( mNumPlaybackChannels, nullptr )
    , mClient( 0 )
    , mInitialised( false )
    , mCapturePorts( mNumCaptureChannels, nullptr )
    , mPlaybackPorts( mNumPlaybackChannels, nullptr )
    {
      JackInterface::Config config = parseSpecificConf(conf);
      
      //            config.loadPortConfigJson(config.mPortJSONConfig, mNumCaptureChannels, mNumPlaybackChannels);
      
      //      if(!config.mPortJSONConfig.empty()){
      //      boost::property_tree::ptree ptree;
      boost::optional<boost::property_tree::ptree> ptree;
      
      ptree = config.mPortJSONConfig.get_child_optional("capture");
      
      config.loadPortConfig(ptree, config.mInExtClientName, config.mCapturePortNames, mInExternalPortNames, mNumCaptureChannels, config.mInAutoConnect, "in_");
      
      ptree = config.mPortJSONConfig.get_child_optional("playback");
      
      config.loadPortConfig(ptree, config.mOutExtClientName, config.mPlaybackPortNames, mOutExternalPortNames, mNumPlaybackChannels, config.mOutAutoConnect, "out_");
      //      }
      if(!config.mClientName.empty()) mClientName = config.mClientName;
      else mClientName = "JackClient";
      
      // Check port name uniqueness (over capture and playback.
      auto allPortNames( mCapturePortNames );
      allPortNames.insert( allPortNames.end(), config.mPlaybackPortNames.begin(),config.mPlaybackPortNames.end() );
      std::sort(allPortNames.begin(),allPortNames.end());
      auto const adjIt = std::adjacent_find(allPortNames.begin(), allPortNames.end());
      if( adjIt != allPortNames.end())
      {
        throw std::invalid_argument( detail::composeMessageString("JackInterface: Duplicate port names not allowed: \"",
                                                                  *adjIt, "\".") );
      }
      
      mCapturePortNames = config.mCapturePortNames;
      mPlaybackPortNames = config.mPlaybackPortNames;
      
      mInExtClientName = config.mInExtClientName;
      mOutExtClientName = config.mOutExtClientName;
      mInAutoConnect = config.mInAutoConnect;
      mOutAutoConnect = config.mOutAutoConnect;
      
      if( mCapturePortNames.size() != mNumCaptureChannels )
      {
        std::cout<<"mCapPorts: "<< mCapturePortNames.size()<<" mCapChan: "<< mNumCaptureChannels<< std::endl;
        throw std::invalid_argument( "JackInterface: The list of capture port names must have the same number of elements as the capture width." );
      }
      if( mPlaybackPortNames.size() != mNumPlaybackChannels )
      {
        throw std::invalid_argument( "JackInterface: The list of playback port names must have the same number of elements as the playback width." );
      }
      
      
      char const * const serverName = config.mServerName.empty() ? "default" : config.mServerName.c_str();
      char const * const clientName = mClientName.c_str();
      
      jack_status_t status;
      jack_options_t options = static_cast<jack_options_t>(JackNoStartServer | JackServerName | JackUseExactName);
      mClient = jack_client_open( clientName, options, &status, serverName );
      if( (status & JackFailure) or !mClient )
      {
        std::stringstream statusMsg;
        bool first = true; // put a comma before each message except the first
        for( JackStatusDesc const & v : cJackStatusTranslator )
        {
          if( std::get<0>( v ) & status )
          {
            if( first )
            {
              first = false;
            }
            else
            {
              statusMsg << ", ";
            }
            statusMsg << std::get<1>( v );
          }
        }
        throw std::invalid_argument( detail::composeMessageString( "JackInterface: Opening of client failed: ", statusMsg.str() ) );
      }
      if( jack_set_process_callback( mClient, &Impl::processCallbackFunction, this ) != 0 )
      {
        throw std::logic_error( "JackInterface: Error registering process callback." );
      }
      if( jack_set_buffer_size_callback( mClient, &Impl::bufferSizeCallbackFunction, this ) != 0 )
      {
        throw std::logic_error( "JackInterface: Error registering buffer size callback." );
      }
      if( jack_set_sample_rate_callback( mClient, &Impl::sampleRateCallbackFunction, this ) != 0 )
      {
        throw std::logic_error( "JackInterface: Error registering sample rate callback." );
      }
      // Note: The shutdown stuff is inconsistent with the other callback/registering interfaces.
      jack_on_shutdown( mClient, &Impl::shutdownCallbackFunction, this );
      if( jack_set_xrun_callback( mClient, &Impl::underrunCallbackFunction, this ) != 0 )
      {
        throw std::logic_error( "JackInterface: Error registering underrun callback." );
      }
      int const res = jack_set_buffer_size( mClient, static_cast<jack_nframes_t>(mPeriodSize) );
      if( res != 0 )
      {
        throw std::logic_error( "JackInterface: Setting the buffer size failed." );
      }
      
      jack_nframes_t const jackSampleRate = jack_get_sample_rate( mClient );
      //            std::cout<<"PERIOD: "<<mPeriodSize<<" SR: "<<jackSampleRate<<std::endl;
      if( jackSampleRate != static_cast<jack_nframes_t>(mSampleRate) )
      {
        throw std::logic_error( "JackInterface: The sample rate of the server differs from the requested sample rate of the client." );
      }
    }
    
    JackInterface::Impl::~Impl()
    {
      stop();
      int const res = jack_client_close( mClient );
      if( res != 0 ) // Destructors mustn't throw
      {
        std::cerr << "JackInterface: Error while closing Jack client." << std::endl;
      }
    }
    
    JackInterface::Config JackInterface::Impl::parseSpecificConf( std::string const & config ){
      //            std::basic_istream<char> stream
      std::string configTr = config;
      boost::trim_if( configTr, boost::is_any_of( "\t " ) );
      std::stringstream stream(configTr);
      //                  std::cout<<"config: "<<config <<" STREAM: "<<stream.str()<<std::endl;
      boost::property_tree::ptree tree;
      if(!configTr.empty()){
        try
        {
          read_json( stream, tree );
        }
        catch( std::exception const & ex )
        {
          throw std::invalid_argument( std::string( "Error while parsing a json node: " ) + ex.what( ) );
        }
      }
      boost::optional<std::string> cliN;
      boost::optional<std::string> servN;
      boost::optional<bool> autoC;
      boost::optional<boost::property_tree::ptree> portsC;
      std::string cliName="";
      std::string servName="";
      bool autoConn = false;
      boost::property_tree::ptree portsConfig;
      
      
      cliN = tree.get_optional<std::string>( "clientname" );
      servN = tree.get_optional<std::string>( "servername" );
      portsC = tree.get_child_optional("portconfig");
      autoC = tree.get_optional<bool>( "autoconnect" );
      
      
      if(cliN) cliName = *cliN;
      if(servN) servName = *servN;
      if(portsC){
        
        //                std::ostringstream oss;
        //                boost::property_tree::ptree ports = *portsC;
        //                boost::property_tree::ini_parser::write_ini(oss, ports);
        //            portsConfig = oss.str();
        portsConfig = *portsC;
        
      }
      if(autoC) autoConn = *autoC;
      //            std::cout<<"CLI: "<<cliName<<" SERV: "<<servName<<" PORTCONF: "<<std::endl;
      return Config(cliName, servName, portsConfig, autoConn);
      
    }
    
    void JackInterface::Impl::registerPorts()
    {
      for( std::size_t captureIdx(0); captureIdx < mNumCaptureChannels; ++captureIdx )
      {
        //      std::cout<<"REGCAPT "<<mCapturePortNames[captureIdx].substr(mCapturePortNames[captureIdx].find(":") + 1).c_str()<<std::endl;
        //                std::cout<< "REGISTERING PORTNAME: "<<mCapturePortNames[captureIdx].c_str()<<std::endl;
        jack_port_t * newPort = jack_port_register( mClient,
                                                   mCapturePortNames[captureIdx].substr(mCapturePortNames[captureIdx].find(":") + 1).c_str(),
                                                   JACK_DEFAULT_AUDIO_TYPE,
                                                   JackPortIsInput,
                                                   0 /* buffer size is ignored for built-in types */ );
        if( newPort == nullptr )
        {
          unregisterPorts();
          throw std::runtime_error( "JackInterface: Registering of port failed." );
        }
        else
        {
          mCapturePorts[captureIdx] = newPort;
        }
      }
      for( std::size_t playbackIdx(0); playbackIdx < mNumPlaybackChannels; ++playbackIdx )
      {
        //      std::cout<<"REGPLAY "<<mPlaybackPortNames[playbackIdx].substr(mPlaybackPortNames[playbackIdx].find(":") + 1).c_str()<<std::endl;
        jack_port_t * newPort = jack_port_register( mClient,
                                                   mPlaybackPortNames[playbackIdx].substr(mPlaybackPortNames[playbackIdx].find(":") + 1).c_str(),
                                                   JACK_DEFAULT_AUDIO_TYPE,
                                                   JackPortIsOutput,
                                                   0 /* buffer size is ignored for built-in types */ );
        if( newPort == nullptr )
        {
          unregisterPorts();
          throw std::runtime_error( "JackInterface: Registering of port failed." );
        }
        else
        {
          mPlaybackPorts[playbackIdx] = newPort;
        }
      }
      
      
      
    }
    
    void JackInterface::Impl::unregisterPorts()
    {
      for( std::size_t captureIdx(0); captureIdx < mNumCaptureChannels; ++captureIdx )
      {
        if( mCapturePorts[captureIdx] == nullptr )
        {
          continue;
        }
        int const res = jack_port_unregister( mClient, mCapturePorts[captureIdx] );
        if( res != 0 )
        {
          std::cerr << "JackInterface: Error while unregistering port" << std::endl;
        }
        mCapturePorts[captureIdx] = nullptr;
      }
      for( std::size_t playbackIdx(0); playbackIdx < mNumPlaybackChannels; ++playbackIdx )
      {
        if( mPlaybackPorts[playbackIdx] == nullptr )
        {
          continue;
        }
        int const res = jack_port_unregister( mClient, mPlaybackPorts[playbackIdx] );
        if( res != 0 )
        {
          std::cerr << "JackInterface: Error while unregistering port" << std::endl;
        }
        mPlaybackPorts[playbackIdx] = nullptr;
      }
    }
    
    void JackInterface::Impl::connectPorts()
    {
      
      //            std::cout<<"Connections: "<<std::endl;
      if(mInAutoConnect){
        
        
        //                const char **ports;
        //                if ((ports = jack_get_ports (mClient, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
        //                    fprintf(stderr, "Cannot find any physical capture ports\n");
        //                    exit(1);
        //                }
        //
        
        for( std::size_t captureIdx(0); captureIdx < mNumCaptureChannels; ++captureIdx )
        {
          char const * name = (mCapturePortNames[captureIdx]).c_str();
          //                std::cout<<ports[captureIdx] << " ----> "<<name << std::endl;
          
          char const * nameExt = (mInExternalPortNames[captureIdx]).c_str();
          std::cout<<nameExt << " ----> "<<name << std::endl;
          if (jack_connect (mClient, nameExt, name)){
            fprintf (stderr, "cannot connect input ports\n");
          }
          //                    if (jack_connect (mClient, ports[captureIdx], name)){
          //                        fprintf (stderr, "cannot connect input ports\n");
          //                    }
        }
        
        //                free (ports);
      }
      if(mOutAutoConnect){
        //                const char **ports;
        
        //                if ((ports = jack_get_ports (mClient, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
        //                    fprintf(stderr, "Cannot find any physical playback ports\n");
        //                    exit(1);
        //                }
        //
        
        for( std::size_t playbackIdx(0); playbackIdx < mNumPlaybackChannels; ++playbackIdx )
        {
          char const * name = (mPlaybackPortNames[playbackIdx]).c_str();
          //                std::cout<<ports[playbackIdx] << " <---- "<<name << std::endl;
          char const * nameExt = (mOutExternalPortNames[playbackIdx]).c_str();
          
          std::cout<<nameExt << " ----> "<<name << std::endl;
          
          if (jack_connect (mClient, name, nameExt)) {
            fprintf (stderr, "cannot connect output ports\n");
          }
          //                    if (jack_connect (mClient, name, ports[playbackIdx])) {
          //                        fprintf (stderr, "cannot connect output ports\n");
          //                    }
        }
        //                free (ports);
      }
    }
    
    void JackInterface::Impl::disconnectPorts()
    {
      if(mInAutoConnect){
        
        for( std::size_t captureIdx(0); captureIdx < mNumCaptureChannels; ++captureIdx )
        {
          char const * name = (mCapturePortNames[captureIdx]).c_str();
          //                std::cout<<ports[captureIdx] << " ----> "<<name << std::endl;
          
          char const * nameExt = (mInExternalPortNames[captureIdx]).c_str();
          
          if (jack_disconnect (mClient, nameExt, name)){
            fprintf (stderr, "cannot disconnect input ports\n");
          }
        }
      }
      
      if(mOutAutoConnect){
        for( std::size_t playbackIdx(0); playbackIdx < mNumPlaybackChannels; ++playbackIdx )
        {
          char const * name = (mPlaybackPortNames[playbackIdx]).c_str();
          //                std::cout<<ports[playbackIdx] << " <---- "<<name << std::endl;
          char const * nameExt = (mOutExternalPortNames[playbackIdx]).c_str();
          
          
          if (jack_disconnect (mClient, name, nameExt)) {
            fprintf (stderr, "cannot disconnect output ports\n");
          }
        }
      }
      
    }
    
    void JackInterface::Impl::start()
    {
      if( mInitialised )
      {
        disconnectPorts();
        unregisterPorts();
        mInitialised = false;
      }
      int const res = jack_activate( mClient );
      if( res != 0 )
      {
        throw std::runtime_error( "JackInterface::Impl::start() returned an error." );
      }
      registerPorts();
      connectPorts();
      mInitialised = true;
    }
    
    void JackInterface::Impl::stop()
    {
      mInitialised = false;
      disconnectPorts();
      unregisterPorts(); // This ensures that the port pointers are properly reset.
      int const res = jack_deactivate( mClient );
      if( res != 0 )
      {
        throw std::runtime_error( "JackInterface::Impl::stop() returned an error." );
      }
    }
    
    bool JackInterface::Impl::registerCallback( AudioCallback callback, void* userData )
    {
      mCallback = callback;
      mCallbackUserData = userData;
      return true;
    }
    
    bool JackInterface::Impl::unregisterCallback( AudioCallback callback )
    {
      if( mCallback == callback )
      {
        mCallback = nullptr;
        mCallbackUserData = nullptr;
        return true;
      }
      else
      {
        return false;
      }
    }
    
    int JackInterface::Impl::bufferSizeChange( jack_nframes_t newBufferSize )
    {
      return 0;
    }
    
    int JackInterface::Impl::sampleRateChange( jack_nframes_t newSampleRate )
    {
      return 0;
    }
    
    void JackInterface::Impl::serverShutdown()
    {
      std::cerr << "JackInterface: Noticed about server shutdown." << std::endl;
    }
    
    int JackInterface::Impl::underrun()
    {
      return 0;
    }
    
    /*static*/ int
    JackInterface::Impl::processCallbackFunction( jack_nframes_t numFrames, void* arg )
    {
      Impl* me = reinterpret_cast<Impl*>( arg );
      try
      {
        me->process( numFrames );
      }
      catch( std::exception const & ex )
      {
        std::cerr << "JackInterface: Error while executing process function: " << ex.what() << std::endl;
        return 1;
      }
      return 0;
    }
    
    /*static*/ int JackInterface::Impl::bufferSizeCallbackFunction( jack_nframes_t nframes, void *arg )
    {
      return static_cast<Impl*>(arg)->bufferSizeChange( nframes );
    }
    
    /*static*/ int JackInterface::Impl::sampleRateCallbackFunction(jack_nframes_t nframes, void *arg)
    {
      return static_cast<Impl*>(arg)->sampleRateChange( nframes );
    }
    
    /*static*/ void JackInterface::Impl::shutdownCallbackFunction( void *arg )
    {
      static_cast<Impl*>(arg)->serverShutdown( );
    }
    
    /*static*/ int JackInterface::Impl::underrunCallbackFunction( void *arg )
    {
      return static_cast<Impl*>(arg)->underrun( );
    }
    
    void JackInterface::Impl::process( jack_nframes_t numFrames )
    {
      // TODO: More flexible handling of the buffer size
      if( static_cast<jack_nframes_t>( mPeriodSize ) != numFrames )
      {
        throw std::logic_error( "JackInterface: Wrong buffer size." );
      }
      
      if( mCallback and mInitialised )
      {
        setCaptureBuffers( numFrames );
        setPlaybackBuffers( numFrames );
        bool res;
        // Exceptions are caught by the calling static function.
        (*mCallback)(mCallbackUserData, &mCaptureSampleBuffers[0], &mPlaybackSampleBuffers[0], res);
      }
      else
      {
        // no registered callback function is no error. We should think about clearing the output buffers.
      }
    }
    
    void JackInterface::Impl::setCaptureBuffers( jack_nframes_t numFrames )
    {
      for( std::size_t captureIdx(0); captureIdx < mCapturePorts.size(); ++captureIdx )
      {
        SampleType const * const capturePtr
        = static_cast<SampleType const * const>(jack_port_get_buffer( mCapturePorts[captureIdx],
                                                                     numFrames ));
        assert( capturePtr );
        mCaptureSampleBuffers[captureIdx] = capturePtr;
      }
    }
    
    void JackInterface::Impl::setPlaybackBuffers( jack_nframes_t numFrames )
    {
      for( std::size_t playbackIdx(0); playbackIdx < mPlaybackPorts.size(); ++playbackIdx )
      {
        SampleType * const playbackPtr
        = static_cast<SampleType * const>(jack_port_get_buffer( mPlaybackPorts[playbackIdx],
                                                               numFrames ));
        assert( playbackPtr );
        mPlaybackSampleBuffers[playbackIdx] = playbackPtr;
      }
    }
    
    /******************************************************************************/
    /* PortAudioInterface implementation                                          */
    
    JackInterface::JackInterface(Configuration const & baseConfig , std::string const & config )
    : mImpl( new Impl( baseConfig, config ) )
    {
    }
    
    JackInterface::~JackInterface( )
    {
      // nothing to be done, as all cleanup is performed in the implementation object.
    }
    
    
    void JackInterface::start()
    {
      mImpl->start();
    }
    
    void JackInterface::stop()
    {
      mImpl->stop();
    }
    
    /*virtual*/ bool
    JackInterface::registerCallback( AudioCallback callback, void* userData )
    {
      return mImpl->registerCallback( callback, userData );
    }
    
    /*virtual*/ bool
    JackInterface::unregisterCallback( AudioCallback callback )
    {
      return mImpl->unregisterCallback( callback );
    }
    
    
    JackInterface::Config::Config(std::string cliName, std::string servName, boost::property_tree::ptree portsConfig,  bool autoConnect)
    : mClientName(cliName)
    , mInAutoConnect(autoConnect)
    , mOutAutoConnect(autoConnect)
    , mServerName(servName)
    , mPortJSONConfig(portsConfig)
    {
      
    }
    
    void JackInterface::Config::loadPortConfig(boost::optional<boost::property_tree::ptree> tree, std::string & extClientName, std::vector< std::string > & portNames, std::vector< std::string > & extPortNames,
                                               std::size_t numPorts, bool & autoConn,  std::string porttype){
      
      boost::optional<bool> autoConnect;
      boost::optional<std::string> baseName;
      boost::optional<std::string> indicesStr;
      boost::optional<std::string> extBaseName;
      boost::optional<std::string> extIndicesStr;
      boost::optional<std::string> extClient;
      
      boost::optional<boost::property_tree::ptree> extPort;
      boost::property_tree::ptree port;
      //                for(boost::property_tree::ptree::value_type &root : tree.get_child(type)){
      //                    for(boost::property_tree::ptree::value_type &t2 : temp.second){
      //                    std::cout<<root.first<< ": "<<root.second.data()<<std::endl;
      
      if(tree){
        autoConnect = (*tree).get_optional<bool>( "autoconnect" );
      }
      if(autoConnect) autoConn = *autoConnect;
      
      
      portNames.resize( numPorts );
      extPortNames.resize( numPorts );
      std::size_t globalIdx( 0 );
      std::size_t extGlobalIdx( 0 );
      
      if(tree){
        if(!(*tree).get_child_optional("port")){
          boost::property_tree::ptree str,chl;
          str.put_child("empty",chl);
          port = str;
          
        }else{
          port = (*tree).get_child("port");
        }
      }else{
        boost::property_tree::ptree str,chl;
        str.put_child("empty",chl);
        port = str;
      }
      for(boost::property_tree::ptree::value_type &pconf : port){
        baseName = pconf.second.get_optional<std::string>( "basename" );
        indicesStr = pconf.second.get_optional<std::string>( "indices" );
        if(indicesStr && baseName){
          rbbl::IndexSequence const inIndices( *indicesStr );
          std::size_t const inNumEntries = inIndices.size( );
          if(inNumEntries > numPorts)
          {
            throw std::invalid_argument( "JackInterface: the number of "+porttype+" ports addressed with indices cannot be greater than the requested number of "+porttype+" ports" );
          }
          for( std::size_t entryIdx( 0 ) ; entryIdx < inNumEntries; ++entryIdx, ++globalIdx )
          {
            std::stringstream str;
            str << mClientName <<":"<< *baseName << inIndices[entryIdx];
            portNames[globalIdx] = str.str();
          }
        }else{
          if(!baseName){
            baseName ="visr_"+porttype;
          }
          
          for(std::size_t entryIdx( 0 ); entryIdx < numPorts; ++entryIdx, ++globalIdx)
          {
            std::stringstream str;
            str << mClientName <<":"<< *baseName << entryIdx;
            portNames[globalIdx] = str.str();
          }
        }
        
        extPort = pconf.second.get_child_optional("externalport");
        if(autoConn){
          if(extPort){
            extClient = (*extPort).get_optional<std::string>( "client" );
            extBaseName = (*extPort).get_optional<std::string>( "portname" );
            extIndicesStr = (*extPort).get_optional<std::string>( "indices" );
          }
          //                                        std::cout<<"TREEDATA: "<< ", "<<tree.data()<<std::endl;
          if(!extClient){
            extClient="system";
          }
          if(!extBaseName){
            if(porttype=="in_") extBaseName="capture_";
            else extBaseName="playback_";
          }
          extClientName = *extClient;
          if(extIndicesStr){
            rbbl::IndexSequence const extInIndices( *extIndicesStr );
            std::size_t const extInNumEntries = extInIndices.size( );
            
            
            for( std::size_t extEntryIdx( 0 ); extEntryIdx < extInNumEntries; ++extEntryIdx,++extGlobalIdx )
            {
              std::stringstream str;
              str << extClientName <<":"<< *extBaseName << extInIndices[extEntryIdx];
              //                               std::cout<<"EXTPORTNAMES: "<<str.str()<<" GLOB: "<<extGlobalIdx<<std::endl;
              extPortNames[extGlobalIdx] = str.str();
            }
          }else{
            
            for( std::size_t extEntryIdx( 0 ); extEntryIdx < numPorts; ++extEntryIdx,++extGlobalIdx )
            {
              std::stringstream str;
              str << extClientName <<":"<< *extBaseName << extEntryIdx+1;
              extPortNames[extGlobalIdx] = str.str();
            }
          }
          
        }
      }
      //      std::cout<<" extglob "<<extGlobalIdx<<" glob "<<globalIdx<<" nump "<<numPorts<<std::endl;
      if(globalIdx != numPorts)
      {
        throw std::invalid_argument( "JackInterface: the total number of "+porttype+" ports addressed with indices must be equal to the requested number of "+porttype+" ports" );
      }
      
      if(autoConn){
        if(extGlobalIdx != numPorts)
        {
          throw std::invalid_argument( "JackInterface: the total number of external "+porttype+" ports addressed with indices must be equal to the requested number of external "+porttype+" ports" );
        }
      }
    }
    
      } // namespace rrl
} // namespace visr
