/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRCL_PANNING_GAIN_CALCULATOR_HPP_INCLUDED
#define VISR_LIBRCL_PANNING_GAIN_CALCULATOR_HPP_INCLUDED

#include <libril/constants.hpp>
#include <libril/audio_component.hpp>

#include <libpanning/LoudspeakerArray.h>
#include <libpanning/VBAP.h>
#include <libpanning/XYZ.h>

#include <memory> // for std::unique_ptr
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
}
namespace ril
{
class AudioInput;
}

namespace rcl
{

/**
 * Audio component for calculating the gains for a variety of panning algorithms from a set of audio object descriptions.
 */
class PanningGainCalculator: public ril::AudioComponent
{
public:
  /**
   * Type of the gain coefficients. We use the same type as
   */
  using CoefficientType = ril::SampleType;

  /**
   * Constructor.
   * @param container A reference to the containing AudioSignalFlow object.
   * @param name The name of the component. Must be unique within the containing AudioSignalFlow.
   */
  explicit PanningGainCalculator( ril::AudioSignalFlow& container, char const * name );

  /**
   * Disabled (deleted) copy constructor
   */
  PanningGainCalculator( PanningGainCalculator const & ) = delete;


  /**
   * Destructor.
   */
  ~PanningGainCalculator();

  /**
   * Method to initialise the component.
   * @param numberOfObjects
   * @param numberOfLoudspeakers
   * @param arrayConfigFile
   */ 
  void setup( std::size_t numberOfObjects, std::size_t numberOfLoudspeakers, std::string const & arrayConfigFile );

  /**
   * The process function. 
   * It takes a vector of objects as input and calculates a vector of output gains.
   */
  void process( objectmodel::ObjectVector const & objects, efl::BasicMatrix<CoefficientType> & gainMatrix );

  /**
   * Set the reference listener position.
   * THe listener postions are used beginning with the next process() call.
   * @note input parameters are likely to be changed to some more sophisticated type (i.e., libpml)
   * @todo Consider extending the interface to multiple listener positions.
   */
  void setListenerPosition( CoefficientType x, CoefficientType y, CoefficientType z );

private:
  std::size_t mNumberOfObjects;

  /**
   * The number of panning loudspeakers.
   * @note This excludes any potential subwoofers (which are not handled by the panninf algorithm)
   */
  std::size_t mNumberOfLoudspeakers;

  /**
   * @todo At the moment, libpanning does not use namespaces.
   * Change accordingly after the library has been adjusted.
   */
  //@{
  LoudspeakerArray mSpeakerArray;

  XYZ mListenerPosition;

  VBAP mVbapCalculator;
  //@}
};

} // namespace rcl
} // namespace visr

#endif // #ifndef VISR_LIBRCL_PANNING_GAIN_CALCULATOR_HPP_INCLUDED
