/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "jack_interface.hpp"

#include <libril/constants.hpp>

#include <jack/jack.h>

#include <cassert>
#include <ciso646> // should not be necessary in C++11, but MSVC is non-compliant here
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace visr
{
namespace rrl
{

/******************************************************************************/
/* Definition of the internal implementation class JackInterface::Impl   */

class JackInterface::Impl
{
public:
  explicit Impl( JackInterface::Config const & config );

  ~Impl( );

  void start();

  void stop();

  bool registerCallback( AudioCallback callback, void* userData );

  bool unregisterCallback( AudioCallback audioCallback );
private:
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

  std::size_t const mPeriodSize;

  std::size_t const mNumCaptureChannels;

  std::size_t const mNumPlaybackChannels;

  std::size_t mSampleRate;

  Base::AudioCallback mCallback;

  void* mCallbackUserData;

  /**
   * Buffer to hold the pointers to the sample vectors for the input
   * of the audio processor.
   * These samples are written from the capture ports of the sound
   * interface and then passed to the audio processing callback function.
   */
  std::vector< ril::SampleType const * > mCaptureSampleBuffers;

  /**
   * Buffer to hold the pointers to the sample vectors for the output
   * of the audio processor.
   * These samples are generated by the audio processing callback
   * function and then passed to the playback argument of the
   * portaudio callback function.
   */
  std::vector< ril::SampleType * > mPlaybackSampleBuffers;

  jack_client_t* mClient;

  bool mInitialised;

  std::vector<std::string> const mCapturePortNames;
  std::vector<std::string> const mPlaybackPortNames;

  std::vector<jack_port_t*> mCapturePorts;

  std::vector<jack_port_t*> mPlaybackPorts;

};

/******************************************************************************/
/* Implementation of the JackInterface::Impl class                       */

JackInterface::Impl::Impl( Config const & config )
 : mPeriodSize( config.mPeriodSize )
 , mNumCaptureChannels( config.mNumberOfCaptureChannels )
 , mNumPlaybackChannels( config.mNumberOfPlaybackChannels )
 , mSampleRate( config.mSampleRate )
 , mCallback( nullptr )
 , mCallbackUserData( nullptr )
 , mCaptureSampleBuffers( mNumCaptureChannels, nullptr )
 , mPlaybackSampleBuffers( mNumPlaybackChannels, nullptr )
 , mClient( 0 )
 , mInitialised( false )
 , mCapturePortNames( config.mCapturePortNames )
 , mPlaybackPortNames( config.mPlaybackPortNames )
 , mCapturePorts( mNumCaptureChannels, nullptr )
 , mPlaybackPorts( mNumPlaybackChannels, nullptr )
{
  if( mCapturePortNames.size() != mNumCaptureChannels )
  {
    throw std::invalid_argument( "JackInterface: The list of capture port names must have the same number of elements as the capture width." );
  }
  if( mPlaybackPortNames.size() != mNumPlaybackChannels )
  {
    throw std::invalid_argument( "JackInterface: The list of playback port names must have the same number of elements as the playback width." );
  }
  char const * const serverName = config.mServerName.empty() ? "default" : config.mServerName.c_str();
  char const * const clientName = config.mClientName.c_str(); 
  jack_status_t status;
  jack_options_t options = static_cast<jack_options_t>(JackNoStartServer | JackServerName | JackUseExactName);
  mClient = jack_client_open( clientName, options, &status, serverName );
  if( (status & JackFailure) or !mClient )
  {
    throw std::invalid_argument( "JackInterface: Opening of client failed." );
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

void JackInterface::Impl::registerPorts()
{
  for( std::size_t captureIdx(0); captureIdx < mNumCaptureChannels; ++captureIdx )
  {
    jack_port_t * newPort = jack_port_register( mClient,
						mCapturePortNames[captureIdx].c_str(),
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
    jack_port_t * newPort = jack_port_register( mClient,
						mPlaybackPortNames[playbackIdx].c_str(),
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
  // not implemented yet.
}

void JackInterface::Impl::disconnectPorts()
{
  // not implemented yet.
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
    CallbackResult res;
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
    ril::SampleType const * const capturePtr
      = static_cast<ril::SampleType const * const>(jack_port_get_buffer( mCapturePorts[captureIdx],
									 numFrames ));
    assert( capturePtr );
    mCaptureSampleBuffers[captureIdx] = capturePtr;
  }
}

void JackInterface::Impl::setPlaybackBuffers( jack_nframes_t numFrames )
{
  for( std::size_t playbackIdx(0); playbackIdx < mPlaybackPorts.size(); ++playbackIdx )
  {
    ril::SampleType * const playbackPtr
      = static_cast<ril::SampleType * const>(jack_port_get_buffer( mPlaybackPorts[playbackIdx],
									 numFrames ));
    assert( playbackPtr );
    mPlaybackSampleBuffers[playbackIdx] = playbackPtr;
  }
}

/******************************************************************************/
/* PortAudioInterface implementation                                          */

JackInterface::JackInterface( Config const & config )
: mImpl( new Impl( config ) )
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

void JackInterface::Config::setCapturePortNames( std::string const baseName,
						 std::size_t startIndex,
						 std::size_t endIndex )
{
  if( startIndex > endIndex )
  {
    throw std::invalid_argument( "JackInterface::Config::setCapturePortNames(): start index exceeds end index." );
  }
  std::size_t const numPorts = endIndex - startIndex + 1;
  mCapturePortNames.resize( numPorts );
  for( std::size_t runIdx( 0 ); runIdx < numPorts; ++runIdx )
  {
    std::stringstream str;
    str << baseName << runIdx;
    mCapturePortNames[runIdx] = str.str();
  }
}

void JackInterface::Config::setPlaybackPortNames( std::string const baseName,
						  std::size_t startIndex, std::size_t endIndex )
{
  if( startIndex > endIndex )
  {
    throw std::invalid_argument( "JackInterface::Config::setPlaybackPortNames(): start index exceeds end index." );
  }
  std::size_t const numPorts = endIndex - startIndex + 1;
  mPlaybackPortNames.resize( numPorts );
  for( std::size_t runIdx( 0 ); runIdx < numPorts; ++runIdx )
  {
    std::stringstream str;
    str << baseName << runIdx;
    mPlaybackPortNames[runIdx] = str.str();
  }
}

} // namespace rrl
} // namespace visr
