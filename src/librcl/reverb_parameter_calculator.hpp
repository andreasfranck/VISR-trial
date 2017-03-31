/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRCL_REVERB_PARAMETER_CALCULATOR_HPP_INCLUDED
#define VISR_LIBRCL_REVERB_PARAMETER_CALCULATOR_HPP_INCLUDED

// Preliminary solution, dependencies between components are suboptimal
#include "late_reverb_filter_calculator.hpp"

#include <libril/atomic_component.hpp>
#include <libril/constants.hpp>

#include <libpml/biquad_parameter.hpp>
#include <libpml/matrix_parameter.hpp>
#include <libpml/message_queue_protocol.hpp>
#include <libpml/object_vector.hpp>
#include <libpml/shared_data_protocol.hpp>
#include <libpml/signal_routing_parameter.hpp>
#include <libpml/vector_parameter.hpp>

#include <libobjectmodel/object.hpp> // needed basically for type definitions

#include <libpanning/LoudspeakerArray.h>
#include <libpanning/VBAP.h>
#include <libpanning/XYZ.h>

#include <vector>

namespace visr
{
// Forward declarations
namespace rbbl
{
  class ObjectChannelAllocator;
}

namespace rcl
{

/**
 * Audio component for calculating the gains for a variety of panning algorithms from a set of audio object descriptions.
 */
class ReverbParameterCalculator: public AtomicComponent
{
public:
  /**
   * Constructor.
   * @param container A reference to the containing AudioSignalFlow object.
   * @param name The name of the component. Must be unique within the containing AudioSignalFlow.
   */
  explicit ReverbParameterCalculator( SignalFlowContext& context,
                                      char const * name,
                                      CompositeComponent * parent = nullptr );

  /**
   * Disabled (deleted) copy constructor
   */
  ReverbParameterCalculator( ReverbParameterCalculator const & ) = delete;


  /**
   * Destructor.
   */
  ~ReverbParameterCalculator();

  /**
   * Method to initialise the component.
   * @param arrayConfig The array configuration object.
   * @param numberOfObjects The maximum number of reverb objects to be processed.
   * @param numberOfDiscreteReflectionsPerSource The number of discrete reflections rendered per source.
   * @param numBiquadSectionsReflectionFilters The number of biquad sections per reflection of the wall reflection filters.
   * @param lateReflectionLengthSeconds The length of the late impulse response representing the late reverberation tail (in seconds).
   * @param numLateReflectionSubBandFilters The number of subbands used in the computation of the late reverb tail. At the moment, this must match a hard-coded limit (9).
   */
  void setup( panning::LoudspeakerArray const & arrayConfig,
              std::size_t numberOfObjects,
              std::size_t numberOfDiscreteReflectionsPerSource,
              std::size_t numBiquadSectionsReflectionFilters,
              SampleType lateReflectionLengthSeconds,
              std::size_t numLateReflectionSubBandFilters );

  /**
   * The process function. 
   */
  void process() override;

private:
  std::unique_ptr<rbbl::ObjectChannelAllocator> mChannelAllocator;

  /**
   * The number of "objects with reverb" handled by this calculator object.
   */
  std::size_t mMaxNumberOfObjects;

  std::size_t mNumberOfDiscreteReflectionsPerSource;
  std::size_t mNumberOfBiquadSectionsReflectionFilters;
  SampleType mLateReflectionLengthSeconds;
  std::size_t mNumberOfLateReflectionSubBandFilters;

  std::size_t mNumberOfPanningLoudspeakers;

  /**
   * A vector to hold the source position data.
   */
  std::vector<panning::XYZ> mSourcePositions;

  /**
   * The calculator object to generate the panning matrix coefficients.
   */
  panning::VBAP mVbapCalculator;

  /**
   * An object holding sensible default values for the late reverb part that
   * result in a zero-valued late reverb tail.
   */
  static const objectmodel::PointSourceWithReverb::LateReverb cDefaultLateReverbParameter;

  /**
   * A table holding the previous states of the reverb parameters for the reverb channel.
   * Used to detect changes in the that trigger an retransmission to the LateReverbFilterCalculator component.
   */
  std::vector<objectmodel::PointSourceWithReverb::LateReverb> mPreviousLateReverbs;

  /**
   * A floating-point limit to compare LateReverb parameters,
   * Two parameters are considered equal if the difference between all corresponding floating-point values is less or equal this value.
   */
  SampleType const cLateReverbParameterComparisonLimit;

  void processSingleObject( objectmodel::PointSourceWithReverb const & rsao, std::size_t renderChannel,
                            efl::BasicVector<SampleType> & discreteReflGains,
                            efl::BasicVector<SampleType> & discreteReflDelays,
                            pml::BiquadParameterMatrix<SampleType> & biquadCoeffs,
                            efl::BasicMatrix<SampleType> & discretePanningMatrix,
                            efl::BasicVector<SampleType> & lateReverbGains,
                            efl::BasicVector<SampleType> & lateReverbDelays );

  // void processInternal( objectmodel::ObjectVector const & objects );

/**
* Set the data members for given reverb object channel to safe, neutral values such that no sound is rendered.
* Used if a render channels is unused.
*/
  void clearSingleObject( std::size_t renderChannel,
                          efl::BasicVector<SampleType> & discreteReflGains,
                          efl::BasicVector<SampleType> & discreteReflDelays,
                          pml::BiquadParameterMatrix<SampleType> & biquadCoeffs,
                          efl::BasicMatrix<SampleType> & discretePanningMatrix,
                          efl::BasicVector<SampleType> & lateReverbGains,
                          efl::BasicVector<SampleType> & lateReverbDelays );

  ParameterInput< pml::SharedDataProtocol, pml::ObjectVector > mObjectInput;
  std::unique_ptr<ParameterOutput < pml::SharedDataProtocol, pml::SignalRoutingParameter > > mSignalRoutingOutput;
  std::unique_ptr<ParameterOutput < pml::SharedDataProtocol, pml::VectorParameter<SampleType> > > mDiscreteReflectionGainOutput;
  std::unique_ptr<ParameterOutput < pml::SharedDataProtocol, pml::VectorParameter<SampleType> > > mDiscreteReflectionDelayOutput;
  std::unique_ptr<ParameterOutput< pml::SharedDataProtocol, pml::BiquadParameterMatrix<SampleType> > > mDiscreteReflectionFilterCoeffOutput;
  std::unique_ptr<ParameterOutput< pml::SharedDataProtocol, pml::MatrixParameter<SampleType> > > mDiscretePanningGains;
  std::unique_ptr<ParameterOutput < pml::SharedDataProtocol, pml::VectorParameter<SampleType> > > mLateReflectionGainOutput;
  std::unique_ptr<ParameterOutput < pml::SharedDataProtocol, pml::VectorParameter<SampleType> > > mLateReflectionDelayOutput;
  std::unique_ptr<ParameterOutput < pml::MessageQueueProtocol, pml::IndexedValueParameter< std::size_t, std::vector<SampleType> > > >
    mLateSubbandOutput;
};

} // namespace rcl
} // namespace visr

#endif // #ifndef VISR_LIBRCL_REVERB_PARAMETER_CALCULATOR_HPP_INCLUDED
