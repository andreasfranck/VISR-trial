/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "bunch_renderer.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cmath>

#include <sstream>
#include <vector>

namespace visr
{
namespace signalflows
{

BunchRenderer::BunchRenderer( SignalFlowContext const & context,
                                    char const * name,
                                    CompositeComponent * parent,
                                    panning::LoudspeakerArray const & loudspeakerConfiguration,
                                    std::size_t numberOfInputs,
                                    std::size_t numberOfOutputs,
                                    std::size_t interpolationPeriod,
                                    efl::BasicMatrix<SampleType> const & diffusionFilters,
                                    std::string const & trackingConfiguration,
                                    std::size_t sceneReceiverPort,
                                    std::string const & reverbConfig )
 : CompositeComponent( context, name, parent )
 , mSceneReceiver( context, "SceneReceiver", this, sceneReceiverPort, rcl::UdpReceiver::Mode::Synchronous )
 , mSceneDecoder( context, "SceneDecoder", this )
 , mCoreRenderer( context, "CoreRenderer", this,
     loudspeakerConfiguration, numberOfInputs, numberOfOutputs,
     interpolationPeriod, diffusionFilters, trackingConfiguration,
     0, reverbConfig, false )
 , mInput( "input", *this )
 , mOutput( "output", *this )
{
  mInput.setWidth(numberOfInputs);
  mOutput.setWidth(numberOfOutputs);

   // mCoreRenderer is initialised in the ctor

  parameterConnection("SceneReceiver", "messageOutput", "SceneDecoder", "datagramInput");
  parameterConnection("SceneDecoder", "objectVectorOutput", "CoreRenderer", "objectDataInput" );

  audioConnection("", "input", ChannelRange(0, numberOfInputs), "CoreRenderer", "audioIn", ChannelRange(0, numberOfInputs) );
  audioConnection("CoreRenderer", "audioOut", ChannelRange(0, numberOfOutputs), "", "output", ChannelRange(0, numberOfInputs) );
}

BunchRenderer::~BunchRenderer( )
{
}


} // namespace signalflows
} // namespace visr
