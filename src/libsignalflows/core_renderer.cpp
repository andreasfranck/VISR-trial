/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "core_renderer.hpp"

#include <libefl/vector_functions.hpp>

#include <libobjectmodel/point_source_with_reverb.hpp>

#include <libpanning/XYZ.h>
#include <libpanning/LoudspeakerArray.h>

#include <libpml/biquad_parameter.hpp>

#include <librcl/biquad_iir_filter.hpp>


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

CoreRenderer::CoreRenderer( SignalFlowContext & context,
                                    char const * name,
                                    CompositeComponent * parent,
                                    panning::LoudspeakerArray const & loudspeakerConfiguration,
                                    std::size_t numberOfInputs,
                                    std::size_t numberOfOutputs,
                                    std::size_t interpolationPeriod,
                                    efl::BasicMatrix<SampleType> const & diffusionFilters,
                                    std::string const & trackingConfiguration )
 : CompositeComponent( context, name, parent )
 , mObjectSignalInput( "audioIn", *this )
 , mLoudspeakerOutput( "audioOut", *this )
 , mObjectVector( "objectDataInput", *this, pml::EmptyParameterConfig() )
 , mDiffusionFilters( diffusionFilters )
 , mOutputAdjustment( context, "OutputAdjustment", this )
 , mGainCalculator( context, "VbapGainCalculator", this )
 , mDiffusionGainCalculator( context, "DiffusionCalculator", this )
 , mVbapMatrix( context, "VbapGainMatrix", this )
 , mDiffusePartMatrix( context, "DiffusePartMatrix", this )
 , mDiffusePartDecorrelator( context, "DiffusePartDecorrelator", this )
 , mDirectDiffuseMix( context, "DirectDiffuseMixer", this, loudspeakerConfiguration.getNumRegularSpeakers(), 2 )
 , mSubwooferMix( context, "SubwooferMixer", this )
 , mNullSource( context, "NullSource", this )
{
  mObjectSignalInput.setWidth( numberOfInputs );
  mLoudspeakerOutput.setWidth( numberOfOutputs );

  std::size_t const numberOfLoudspeakers = loudspeakerConfiguration.getNumRegularSpeakers();
  std::size_t const numberOfSubwoofers = loudspeakerConfiguration.getNumSubwoofers();
  std::size_t const numberOfOutputSignals = numberOfLoudspeakers + numberOfSubwoofers;

  mTrackingEnabled = not trackingConfiguration.empty( );
  if( mTrackingEnabled )
  {
    // Instantiate the objects.
    mListenerPositionPort.reset( new ParameterInput< pml::MessageQueueProtocol, pml::ListenerPosition >( "listenerPositionInput", *this, pml::EmptyParameterConfig() ) );
    mListenerCompensation.reset( new rcl::ListenerCompensation( context, "TrackingListenerCompensation" ) );
    mSpeakerCompensation.reset( new rcl::DelayVector( context, "TrackingSpeakerCompensation" ) );
    mPositionDecoder.reset( new rcl::PositionDecoder( context, "TrackingPositionDecoder" ) );

    // for the very moment, do not parse any options, but use hard-coded option values.
    SampleType const cMaxDelay = 1.0f; // maximum delay (in seconds)
    mListenerCompensation->setup( loudspeakerConfiguration );
    // We start with a initial gain of 0.0 to suppress transients on startup.
    mSpeakerCompensation->setup( numberOfLoudspeakers, period(), cMaxDelay,
      rcl::DelayVector::InterpolationType::NearestSample,
      0.0f, 0.0f );
    mPositionDecoder->setup( panning::XYZ( +2.08f, 0.0f, 0.0f ) );
  }

  bool const outputEqSupport = loudspeakerConfiguration.outputEqualisationPresent();
  if( outputEqSupport )
  {
    std::size_t const outputEqSections = loudspeakerConfiguration.outputEqualisationNumberOfBiquads();
    pml::BiquadParameterMatrix<Afloat> const & eqConfig = loudspeakerConfiguration.outputEqualisationBiquads();
    if( numberOfOutputSignals != eqConfig.numberOfFilters() )
    {
      throw std::invalid_argument( "CoreRenderer: Size of the output EQ configuration config differs from "
        "the number of output signals (regular loudspeakers + subwoofers).");
    }
    mOutputEqualisationFilter.reset( new rcl::BiquadIirFilter( context, "OutputEqualisationFilter" ) );
    mOutputEqualisationFilter->setup( numberOfOutputSignals, outputEqSections, eqConfig );
  }

  mGainCalculator.setup( numberOfInputs, loudspeakerConfiguration );
  parameterConnection( "this", "objectDataInput", "VbapGainCalculator", "objectVectorInput" );
  mVbapMatrix.setup( numberOfInputs, numberOfLoudspeakers, interpolationPeriod, 0.0f );
  parameterConnection( "VbapGainCalculator", "gainOutput", "VbapGainMatrix", "gainInput" );

  mDiffusionGainCalculator.setup( numberOfInputs );
  parameterConnection("this", "objectDataInput", "DiffusionCalculator", "objectInput" );
  mDiffusePartMatrix.setup( numberOfInputs, 1, interpolationPeriod, 0.0f );
  parameterConnection( "DiffusionCalculator", "gainOutput", "DiffusePartMatrix", "gainInput" );


  /**
   * Adjust the level of the diffuse objects such that they are comparable to point sources.
   * Here we assume that the decorrelated signals are ideally decorrelated. Note that this is not 
   * the case with the current set of decorrelation filters.
   * @todo Also consider a more elaborate panning law between the direct and diffuse part of a single source. 
   */
  SampleType const diffusorGain = static_cast<SampleType>(1.0) / std::sqrt( static_cast<SampleType>(numberOfLoudspeakers) );
  mDiffusePartDecorrelator.setup( numberOfLoudspeakers, mDiffusionFilters, diffusorGain );
  mNullSource.setup( 1/*width*/ );

  efl::BasicVector<SampleType> const & outputGains =loudspeakerConfiguration.getGainAdjustment();
  efl::BasicVector<SampleType> const & outputDelays = loudspeakerConfiguration.getDelayAdjustment();
  
  Afloat const * const maxEl = std::max_element( outputDelays.data(),
                                                outputDelays.data()+outputDelays.size() );
  Afloat const maxDelay = std::ceil( *maxEl ); // Sufficient for nearestSample even if there is no particular compensation for the interpolation method's delay inside.
  
  mOutputAdjustment.setup( numberOfOutputSignals, period(), maxDelay, rcl::DelayVector::InterpolationType::NearestSample,
    outputDelays, outputGains );

  // Note: This assumes that the type 'Afloat' used in libpanning is
  // identical to SampleType (at the moment, both are floats).
  efl::BasicMatrix<SampleType> const & subwooferMixGains = loudspeakerConfiguration.getSubwooferGains();
  mSubwooferMix.setup( numberOfLoudspeakers, numberOfSubwoofers, 0/*interpolation steps*/, subwooferMixGains, false/*controlInput*/ );

  audioConnection( mObjectSignalInput, mVbapMatrix.audioPort( "in" ) );
  audioConnection( mObjectSignalInput, mDiffusePartMatrix.audioPort( "in" ) );
  audioConnection( mVbapMatrix.audioPort( "out" ), mDirectDiffuseMix.audioPort( "in0" ) );
  audioConnection( mDirectDiffuseMix.audioPort( "out" ), mDiffusePartDecorrelator.audioPort( "in" ) );
  audioConnection( "DiffusePartDecorrelator", "out", ChannelRange( 0, numberOfLoudspeakers ), "DirectDiffuseMixer", "in1", ChannelRange( 0, numberOfLoudspeakers ) );
  if( mTrackingEnabled )
  {
    audioConnection( "DirectDiffuseMixer", "out", ChannelRange( 0, numberOfLoudspeakers ), "TrackingSpeakerCompensation", "in", ChannelRange( 0, numberOfLoudspeakers ) );
    audioConnection( "TrackingSpeakerCompensation", "out", ChannelRange( 0, numberOfLoudspeakers ), "SubwooferMixer", "in", ChannelRange( 0, numberOfLoudspeakers ) );
    if( outputEqSupport )
    {
      audioConnection( "TrackingSpeakerCompensation", "out", ChannelRange( 0, numberOfLoudspeakers ), "OutputEqualisationFilter", "in", ChannelRange( 0, numberOfLoudspeakers ) );
    }
    else
    {
      audioConnection( "TrackingSpeakerCompensation", "out", ChannelRange( 0, numberOfLoudspeakers ), "OutputAdjustment", "in", ChannelRange( 0, numberOfLoudspeakers ) );
    }
    parameterConnection( "", "listenerPositionInput", "TrackingListenerCompensation", "input" );
  }
  else
  {
    audioConnection( "DirectDiffuseMixer", "out", ChannelRange( 0, numberOfLoudspeakers ), "SubwooferMixer", "in", ChannelRange( 0, numberOfLoudspeakers ) );
  }
  if( outputEqSupport )
  {
    audioConnection( "DirectDiffuseMixer", "out", ChannelRange( 0, numberOfLoudspeakers ), "OutputEqualisationFilter", "in", ChannelRange( 0, numberOfLoudspeakers ) );
    audioConnection( "SubwooferMixer", "out", ChannelRange( 0, numberOfSubwoofers ),
                             "OutputEqualisationFilter", "in", ChannelRange( numberOfLoudspeakers, numberOfLoudspeakers + numberOfSubwoofers ) );
    audioConnection( "OutputEqualisationFilter", "out", ChannelRange( 0, numberOfLoudspeakers + numberOfSubwoofers ),
                             "OutputAdjustment", "in", ChannelRange( 0, numberOfLoudspeakers + numberOfSubwoofers ) );
  }
  else
  {
    audioConnection( "DirectDiffuseMixer", "out", ChannelRange( 0, numberOfLoudspeakers ), "OutputAdjustment", "in", ChannelRange( 0, numberOfLoudspeakers ) );
    audioConnection( "SubwooferMixer", "out", ChannelRange( 0, numberOfSubwoofers ),
                             "OutputAdjustment", "in", ChannelRange( numberOfLoudspeakers, numberOfLoudspeakers + numberOfSubwoofers ) );
  }
  // Connect to the external playback channels, including the silencing of unused channels.
  if( numberOfLoudspeakers + numberOfSubwoofers > numberOfOutputs ) // Otherwise the computation below would cause an immense memory allocation.
  {
    throw std::invalid_argument( "The number of loudspeakers plus subwoofers exceeds the number of output channels." );
  }
  constexpr ChannelList::IndexType invalidIdx = std::numeric_limits<ChannelList::IndexType>::max();
  std::vector<ChannelList::IndexType> activePlaybackChannels( numberOfLoudspeakers + numberOfSubwoofers, invalidIdx );
  for( std::size_t idx( 0 ); idx < numberOfLoudspeakers; ++idx )
  {
    panning::LoudspeakerArray::ChannelIndex const chIdx = loudspeakerConfiguration.channelIndex( idx );
    if( (chIdx < 0) or (chIdx >= static_cast<panning::LoudspeakerArray::ChannelIndex>(numberOfOutputs)) )
    {
      throw std::invalid_argument( "The loudspeakers channel index exceeds the admissible range." );
    }
    // This does not check whether an index is used multiple times.
    activePlaybackChannels[idx] = chIdx;
  }
  for( std::size_t idx( 0 ); idx < numberOfSubwoofers; ++idx )
  {
    panning::LoudspeakerArray::ChannelIndex const chIdx = loudspeakerConfiguration.getSubwooferChannels()[idx];
    if( (chIdx <= 0) or (chIdx >=  static_cast<panning::LoudspeakerArray::ChannelIndex>(numberOfOutputs) ) )
    {
      throw std::invalid_argument( "The subwoofer channel index exceeds the admissible range." );
    }
    // This does not check whether an index is used multiple times.
    activePlaybackChannels[numberOfLoudspeakers + idx] = chIdx;
  }
  if( std::find( activePlaybackChannels.begin(), activePlaybackChannels.end(), invalidIdx ) != activePlaybackChannels.end() )
  {
    throw std::invalid_argument( "Not all active output channels are assigned." );
  }
  std::vector<CompositeComponent::ChannelList::IndexType> sortedPlaybackChannels( activePlaybackChannels );
  std::sort( sortedPlaybackChannels.begin(), sortedPlaybackChannels.end() );
  if( std::unique( sortedPlaybackChannels.begin(), sortedPlaybackChannels.end() ) != sortedPlaybackChannels.end() )
  {
    throw std::invalid_argument( "The loudspeaker array contains a duplicated output channel index." );
  }
  audioConnection( "OutputAdjustment", "out", ChannelRange(0, numberOfLoudspeakers + numberOfSubwoofers ),
                           "", "audioOut", activePlaybackChannels );

  std::size_t const numSilentOutputs = numberOfOutputs - (numberOfLoudspeakers + numberOfSubwoofers);
  if( numSilentOutputs > 0 )
  {
    std::vector<ChannelList::IndexType> nullOutput( numSilentOutputs, 0 );
    std::vector<ChannelList::IndexType> silencedPlaybackChannels( numSilentOutputs, invalidIdx );
    std::size_t silentIdx = 0;
    std::vector<ChannelList::IndexType>::const_iterator runIt{ sortedPlaybackChannels.begin() };
    for( std::size_t idx( 0 ); idx < numberOfOutputs; ++idx )
    {
      if( runIt == sortedPlaybackChannels.end() )
      {
        silencedPlaybackChannels[silentIdx++] = idx;
        continue;
      }
      if( idx < *runIt )
      {
        silencedPlaybackChannels[silentIdx++] = idx;
      }
      else
      {
        runIt++;
      }
    }
    if( silentIdx != numSilentOutputs )
    {
      throw std::logic_error( "Internal logic error: Computation of silent output channels failed." );
    }
    audioConnection( "NullSource", "out", nullOutput,
                             "", "output", silencedPlaybackChannels );
  }
}

CoreRenderer::~CoreRenderer( )
{
}

} // namespace signalflows
} // namespace visr
