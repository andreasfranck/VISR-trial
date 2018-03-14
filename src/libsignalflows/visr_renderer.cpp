/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "visr_renderer.hpp"

#include <librcl/scene_decoder.hpp>

#include <librcl/python_wrapper.hpp>

#include <boost/format.hpp>

namespace visr
{
namespace signalflows
{

VisrRenderer::VisrRenderer( SignalFlowContext const & context,
                            char const * name,
                            CompositeComponent * parent,
                            panning::LoudspeakerArray const & loudspeakerConfiguration,
                            std::size_t numberOfInputs,
                            std::size_t numberOfOutputs,
                            std::size_t interpolationPeriod,
                            efl::BasicMatrix<SampleType> const & diffusionFilters,
                            std::string const & trackingConfiguration,
                            std::size_t sceneReceiverPort,
                            std::size_t numberOfObjectEqSections,
                            std::string const & reverbConfig,
                            bool frequencyDependentPanning,
                            std::string const & metadapterConfig
                            )
 : CompositeComponent( context, name, parent )
 , mSceneReceiver( context, "SceneReceiver", this )
 , mSceneDecoder( nullptr )
 , mCoreRenderer( context, "CoreRenderer", this, loudspeakerConfiguration, numberOfInputs, numberOfOutputs,
                  interpolationPeriod, diffusionFilters, trackingConfiguration, numberOfObjectEqSections,
                  reverbConfig, frequencyDependentPanning )
 , mInput( "input", *this, numberOfInputs )
 , mOutput( "output", *this, numberOfOutputs )

{
  mSceneReceiver.setup( sceneReceiverPort, rcl::UdpReceiver::Mode::Asynchronous );
  if( metadapterConfig.empty() )
  {
    // std::make_unique (C++14) would be handy.
    mSceneDecoder.reset( new rcl::SceneDecoder( context, "SceneDeoder", this ) );
    parameterConnection( mSceneReceiver.parameterPort( "messageOutput" ), mSceneDecoder->parameterPort( "datagramInput" ) );
    parameterConnection( mSceneDecoder->parameterPort( "objectVectorOutput" ), mCoreRenderer.parameterPort( "objectDataInput" ) );
  }
  else
  {
#if 1 // VISR_PYTHON_SUPPORT
    //  char const * moduleName,
    //  char const * componentClassName,
    //  char const * positionalArguments = "",
    //  char const * keywordArguments = "",
    //  char const * moduleSearchPath = "");

    std::string const formatString = "{'processorConfig': '%s', 'objectVectorInput': True, 'objectVectorOutput': True, 'oscControlPort': False, 'jsonControlPort': False }";
    std::string const kwArgs = str( boost::format( formatString ) % metadapterConfig );

    mSceneDecoder.reset( new rcl::PythonWrapper( context, "Metadapter", this,
      "metadapter", "Metadapter",
      "",
      //(std::string("'") + metadapterConfig + "',").c_str(), // The trailing comma ensures that it can be parsed as a Python tuple.
      //"{'objectVectorInput': True, 'objectVectorOutput': True, 'oscControlPort': False, 'jsonControlPort': False }",
      kwArgs.c_str(),
      "" // No module search path
      ) );
    parameterConnection( mSceneReceiver.parameterPort( "messageOutput" ), mSceneDecoder->parameterPort( "objectIn" ) );
    parameterConnection( mSceneDecoder->parameterPort( "objectOut" ), mCoreRenderer.parameterPort( "objectDataInput" ) );
#else
    throw std::invalid_argument( "Providing a metadapter configuration requires a VISR built with Python support." )
#endif
  }


  audioConnection( mInput, mCoreRenderer.audioPort( "audioIn") );
  audioConnection( mCoreRenderer.audioPort( "audioOut"), mOutput );

  if( not trackingConfiguration.empty() )
  {
    mTrackingReceiver.reset( new rcl::UdpReceiver( context, "TrackingReceiver", this ) );
    mTrackingPositionDecoder.reset( new rcl::PositionDecoder( context, "TrackingPositionDecoder", this, panning::XYZ( 0.0f, 0.0f, 0.0f ) ) );

    mTrackingReceiver->setup( 8888, rcl::UdpReceiver::Mode::Synchronous );
    parameterConnection( mTrackingPositionDecoder->parameterPort("positionOutput"), mCoreRenderer.parameterPort("trackingPositionInput") );
  }
}

VisrRenderer::~VisrRenderer( )
{
}

} // namespace signalflows
} // namespace visr
