/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRCL_DIFFUSION_GAIN_CALCULATOR_HPP_INCLUDED
#define VISR_LIBRCL_DIFFUSION_GAIN_CALCULATOR_HPP_INCLUDED

#include <libril/constants.hpp>
#include <libril/audio_component.hpp>

#include <libobjectmodel/object.hpp> // needed basically for type definitions

#include <vector>

namespace visr
{
// forward declarations
namespace objectmodel
{
class ObjectVector;
}
namespace efl
{
template< typename SampleType > class BasicMatrix;
template< typename SampleType > class BasicVector;
}

namespace rcl
{

/**
 * Audio component for extracting the diffuseness gain from an object vector.
 */
class DiffusionGainCalculator: public ril::AudioComponent
{
public:
  /**
   * Type of the gain coefficients. We use the same type as the audio samples.
   */
  using CoefficientType = ril::SampleType;

  /**
   * Constructor.
   * @param container A reference to the containing AudioSignalFlow object.
   * @param name The name of the component. Must be unique within the containing AudioSignalFlow.
   */
  explicit DiffusionGainCalculator( ril::AudioSignalFlow& container, char const * name );

  /**
   * Disabled (deleted) copy constructor
   */
  DiffusionGainCalculator( DiffusionGainCalculator const & ) = delete;


  /**
   * Destructor.
   */
  ~DiffusionGainCalculator();

  /**
   * Method to initialise the component.
   * @param numberOfObjectChannels The number of object channels supported by this calculator.
   */ 
  void setup( std::size_t numberOfObjectChannels );

  /**
   * The process function. 
   * It takes a vector of objects as input and calculates a vector of output gains. This variant is to enable the use of a GainMatrix component.
   * @param objects The vector of objects. It must consist only of single-channel objects with channel IDs 0...numberOfChannelObjects-1.
   * @ param[out] The vector of diffusion gains for the audio channels. Must be a BasicMatrix with dimension 1 x numberOfChannelObjects.
   */
  void process( objectmodel::ObjectVector const & objects, efl::BasicMatrix<CoefficientType> & gainMatrix );

  /**
  /**
  * The process function.
  * It takes a vector of objects as input and calculates a vector of output gains.
  * @param objects The vector of objects. It must consist only of single-channel objects with channel IDs 0...numberOfChannelObjects-1.
  * @ param[out] The vector of diffusion gains for the audio channels. Must be a vector of size numberOfChannelObjects.
  */
  void process( objectmodel::ObjectVector const & objects, efl::BasicVector<CoefficientType> & gainVector );

private:
  /**
   * The number of audio object channels handled by this object.
   */
  std::size_t mNumberOfObjectChannels;

  /**
   * Internal implementation method for the common part of both process() function.
   */
  void processInternal( objectmodel::ObjectVector const & objects, CoefficientType * gains );

};

} // namespace rcl
} // namespace visr

#endif // #ifndef VISR_LIBRCL_DIFFUSION_GAIN_CALCULATOR_HPP_INCLUDED
