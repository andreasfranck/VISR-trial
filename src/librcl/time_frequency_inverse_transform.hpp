/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRCL_TIME_FREQUENCY_INVERSE_TRANSFORM_HPP_INCLUDED
#define VISR_LIBRCL_TIME_FREQUENCY_INVERSE_TRANSFORM_HPP_INCLUDED

#include "export_symbols.hpp"

#include <libril/atomic_component.hpp>
#include <libril/constants.hpp>
#include <libril/audio_output.hpp>
#include <libril/parameter_input.hpp>

#include <libefl/aligned_array.hpp>
#include <libefl/basic_matrix.hpp>

#include <libpml/time_frequency_parameter.hpp>
#include <libpml/shared_data_protocol.hpp>

#include <cstddef> // for std::size_t

namespace visr
{

// Forward declarations
namespace rbbl
{
template< typename SampleType >
class FftWrapperBase;

template< typename SampleType >
class CircularBuffer;

}

namespace rcl
{

class VISR_RCL_LIBRARY_SYMBOL TimeFrequencyInverseTransform: public AtomicComponent
{
  using SampleType = visr::SampleType;
public:
  /**
   * Constructor.
   * @param context Configuration object containing basic execution parameters.
   * @param name The name of the component. Must be unique within the containing composite component (if there is one).
   * @param parent Pointer to a containing component if there is one. Specify \p nullptr in case of a top-level component
   */
  explicit TimeFrequencyInverseTransform( SignalFlowContext const & context,
                                          char const * name,
                                          CompositeComponent * parent,
                                          std::size_t numberOfChannels,
                                          std::size_t dftLength,
                                          std::size_t hopSize,
                                          char const * fftImplementation = "default" );

  ~TimeFrequencyInverseTransform();

  void process( );

private:
  std::size_t const mAlignment;

  /**
   * 
   */
  std::size_t const mNumberOfChannels;

  /**
   * The length of the Fourier transform;
   */
  std::size_t const mDftLength;

  std::size_t const mDftSamplesPerPeriod;

  std::size_t const mHopSize;

  efl::BasicMatrix<SampleType> mAccumulationBuffer;

  /**
   * Wrapper for the different FFT libraries
   */
  std::unique_ptr< rbbl::FftWrapperBase<SampleType> > mFftWrapper;

  efl::AlignedArray<SampleType> mCalcBuffer;

  /**
  * 
  */
  ParameterInput<pml::SharedDataProtocol, pml::TimeFrequencyParameter<SampleType> > mInput;

  AudioOutput mOutput;
};

} // namespace rcl
} // namespace visr
  
#endif // #ifndef VISR_LIBRCL_TIME_FREQUENCY_INVERSE_TRANSFORM_HPP_INCLUDED
