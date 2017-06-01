/* Copyright Institute of Sound and Vibration Research - All rights reserved */

// Enable native JACK interface instead of PortAudio
// TODO: Make this selectable via a command line option.
// #define BASELINE_RENDERER_NATIVE_JACK

#include "options.hpp"

#include <libril/signal_flow_context.hpp>

#ifdef BASELINE_RENDERER_NATIVE_JACK
#include <libaudiointerfaces/jack_interface.hpp>
#else
#include <libaudiointerfaces/portaudio_interface.hpp>
#endif
#include <librrl/audio_signal_flow.hpp>
#include <librrl/audio_interface.hpp>
#include <libsignalflows/bunch_renderer.hpp>

#include <libefl/denormalised_number_handling.hpp>

#include <boost/algorithm/string.hpp> // case-insensitive string compare
#include <boost/filesystem.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdio> // for getc(), for testing purposes
#include <iostream>
#include <sstream>

int main( int argc, char const * const * argv )
{
  using namespace visr;
  using namespace visr::apps::baseline_renderer;

  try
  {
    efl::DenormalisedNumbers::State const oldDenormNumbersState
    = efl::DenormalisedNumbers::setDenormHandling();

    Options cmdLineOptions;
    std::stringstream errMsg;
    switch( cmdLineOptions.parse( argc, argv, errMsg ) )
    {
      case Options::ParseResult::Failure:
        std::cerr << "Error while parsing command line options: " << errMsg.str() << std::endl;
        return EXIT_FAILURE;
      case Options::ParseResult::Help:
        cmdLineOptions.printDescription( std::cout );
        return EXIT_SUCCESS;
      case Options::ParseResult::Version:
        // TODO: Implement retrieval of version information.
        std::cout << "VISR S3A Baseline Renderer V0.1b" << std::endl;
        return EXIT_SUCCESS;
      case Options::ParseResult::Success:
        break; // carry on
    }

    boost::filesystem::path const arrayConfigPath( cmdLineOptions.getOption<std::string>( "array-config" ) );
    if( !exists( arrayConfigPath ) )
    {
      std::cerr << "The specified loudspeaker array configuration file \""
          << arrayConfigPath.string() << "\" does not exist." << std::endl;
      return EXIT_FAILURE;
    }
    std::string const arrayConfigFileName = arrayConfigPath.string();
    panning::LoudspeakerArray loudspeakerArray;
    // As long as we have two different config formats, we decide based on the file extention.
    std::string::size_type lastDotIdx = arrayConfigFileName.rfind( '.' );
    std::string const configfileExtension = lastDotIdx == std::string::npos ? std::string( ) : arrayConfigFileName.substr( lastDotIdx + 1 );
    if( boost::iequals( configfileExtension, std::string( "xml" ) ) )
    {
      loudspeakerArray.loadXmlFile( arrayConfigFileName );
    }
    else
    {
      FILE* hFile = fopen( arrayConfigFileName.c_str( ), "r" );
      if( loudspeakerArray.load( hFile ) < 0 )
      {
        throw std::invalid_argument( "Error while parsing the loudspeaker array configuration file \""
          + arrayConfigFileName + "\"." );
      }
    }

    const std::size_t numberOfLoudspeakers = loudspeakerArray.getNumRegularSpeakers();
    const std::size_t numberOfSpeakersAndSubs = numberOfLoudspeakers + loudspeakerArray.getNumSubwoofers( );

    const std::size_t numberOfOutputChannels
    = cmdLineOptions.getDefaultedOption<std::size_t>( "output-channels", numberOfSpeakersAndSubs );

    const std::size_t numberOfObjects = cmdLineOptions.getOption<std::size_t>( "input-channels" );
    const std::size_t periodSize = cmdLineOptions.getDefaultedOption<std::size_t>( "period", 1024 );
    const std::size_t samplingRate = cmdLineOptions.getDefaultedOption<std::size_t>( "sampling-frequency", 48000 );

    const std::string audioBackend = cmdLineOptions.getDefaultedOption<std::string>( "audio-backend", "default" );

    const std::size_t  sceneReceiverPort = cmdLineOptions.getDefaultedOption<std::size_t>( "scene-port", 4242 );

    const std::string trackingConfiguration = cmdLineOptions.getDefaultedOption<std::string>( "tracking", std::string() );

    if( not cmdLineOptions.hasOption( "reverb-config" ) )
    {
      // throw std::invalid_argument( "VISR renderer: Mandatory option \"reverb-config\" missing." );
    }
    const std::string reverbConfiguration= cmdLineOptions.getDefaultedOption<std::string>( "reverb-config", std::string() );

#ifdef BASELINE_RENDERER_NATIVE_JACK
    audiointerfaces::JackInterface::Config interfaceConfig;
#else
    audiointerfaces::PortaudioInterface::Config interfaceConfig;
#endif
    interfaceConfig.mNumberOfCaptureChannels = numberOfObjects;
    interfaceConfig.mNumberOfPlaybackChannels = numberOfOutputChannels;
    interfaceConfig.mPeriodSize = periodSize;
    interfaceConfig.mSampleRate = samplingRate;
#ifdef BASELINE_RENDERER_NATIVE_JACK
    interfaceConfig.setCapturePortNames( "input_", 0, numberOfObjects-1 );
    interfaceConfig.setPlaybackPortNames( "output_", 0, numberOfOutputChannels-1 );
    interfaceConfig.mClientName = "BunchRenderer";
#else
    interfaceConfig.mInterleaved = false;
    interfaceConfig.mSampleFormat = audiointerfaces::PortaudioInterface::Config::SampleFormat::float32Bit;
    interfaceConfig.mHostApi = audioBackend;
#endif

    // Assume a fixed length for the interpolation period.
    // Ideally, this roughly matches the update rate of the scene sender.
    const std::size_t cInterpolationLength = 2048;

    /* Set up the filter matrix for the diffusion filters. */
    std::size_t const diffusionFilterLength = 63; // fixed filter length of the filters in the compiled-in matrix
    std::size_t const diffusionFiltersInFile = 64; // Fixed number of filters in file.
    // First create a filter matrix containing all filters from a initializer list that is compiled into the program.
    efl::BasicMatrix<SampleType> allDiffusionCoeffs( diffusionFiltersInFile,
                                                          diffusionFilterLength,
#include "files/quasiAllpassFIR_f64_n63_initializer_list.txt"
                                                          , cVectorAlignmentSamples );

    // Create a second filter matrix that matches the number of required filters.
    efl::BasicMatrix<SampleType> diffusionCoeffs( numberOfLoudspeakers, diffusionFilterLength, cVectorAlignmentSamples );
    for( std::size_t idx( 0 ); idx < diffusionCoeffs.numberOfRows( ); ++idx )
    {
      efl::vectorCopy( allDiffusionCoeffs.row( idx ), diffusionCoeffs.row( idx ), diffusionFilterLength, cVectorAlignmentSamples );
    }

    SignalFlowContext context( periodSize, samplingRate );

    signalflows::BunchRenderer flow( context,
                                     "", nullptr,
                                     loudspeakerArray,
                                     numberOfObjects,
                                     numberOfOutputChannels,
                                     cInterpolationLength,
                                     diffusionCoeffs,
                                     trackingConfiguration,
                                     sceneReceiverPort,
                                     reverbConfiguration );

    rrl::AudioSignalFlow audioFlow( flow );
    visr::rrl::AudioInterface::Configuration const baseConfig(numberOfObjects,numberOfOutputChannels,periodSize,samplingRate);
      
#ifdef BASELINE_RENDERER_NATIVE_JACK
    audiointerfaces::JackInterface audioInterface( baseConfig, interfaceConfig );
#else
    audiointerfaces::PortaudioInterface audioInterface( baseConfig, interfaceConfig );
#endif

    audioInterface.registerCallback( &rrl::AudioSignalFlow::processFunction, &audioFlow );

    // should there be a separate start() method for the audio interface?
    audioInterface.start( );

    // Rendering runs until q<Return> is entered on the console.
    std::cout << "S3A baseline renderer running. Press \"q<Return>\" or Ctrl-C to quit." << std::endl;
    char c;
    do
    {
      c = std::getc( stdin );
    }
    while( c != 'q' );

    audioInterface.stop( );

   // Should there be an explicit stop() method for the sound interface?

//    audioInterface.unregisterCallback( &AudioSignalFlow::processFunction );

    efl::DenormalisedNumbers::resetDenormHandling( oldDenormNumbersState );
  }
  catch( std::exception const & ex )
  {
    std::cout << "Exception caught on top level: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
