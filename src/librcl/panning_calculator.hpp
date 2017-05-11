/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRCL_PANNING_CALCULATOR_HPP_INCLUDED
#define VISR_LIBRCL_PANNING_CALCULATOR_HPP_INCLUDED

#include <libril/atomic_component.hpp>
#include <libril/constants.hpp>
#include <libril/parameter_input.hpp>
#include <libril/parameter_output.hpp>

#include <libefl/basic_matrix.hpp>
#include <libefl/basic_vector.hpp>

#include <libobjectmodel/object.hpp> // needed basically for type definitions

#include <libpanning/LoudspeakerArray.h>
#include <libpanning/VBAP.h>
#include <libpanning/XYZ.h>



#include <memory>
#include <valarray>
#include <vector>

namespace visr
{

namespace panning
{
  class LoudspeakerArray;
}
  
namespace pml
{
class ListenerPosition;
class ObjectVector;
template< typename ElementType > class MatrixParameter;
class SharedDataProtocol;
class DoubleBufferingProtocol;
}

namespace rcl
{

/**
 * Audio component for calculating the gains for a variety of panning algorithms from a set of audio object descriptions.
 */
class PanningCalculator: public AtomicComponent
{
public:
  /**
   * Type of the gain coefficients. We use the same type as the samples in the signal flow graph
   * @todo maybe this should become a template parameter.
   */
  using CoefficientType = SampleType;

  /**
   * Constructor.
   * @param container A reference to the containing AudioSignalFlow object.
   * @param name The name of the component. Must be unique within the containing AudioSignalFlow.
   */
  explicit PanningCalculator( SignalFlowContext const & context,
                              char const * name,
                              CompositeComponent * parent = nullptr );

  /**
   * Disabled (deleted) copy constructor
   */
  PanningCalculator( PanningCalculator const & ) = delete;


  /**`
   * Destructor.
   */
  ~PanningCalculator();

  /**
   * Method to initialise the component.
   * @param numberOfObjects The number of VBAP objects to be processed.
   * @param arrayConfig The array configuration object.
   * @param adaptiveListenerPosition Whether the rendering supports adaptation to a tracked listener.
   */ 
  void setup( std::size_t numberOfObjects,
              panning::LoudspeakerArray const & arrayConfig,
              bool adaptiveListenerPosition=false,
              bool separateLowpassPanning = false );

  /**
   * The process function. 
   * It takes a vector of objects as input and calculates a vector of output gains.
   */
  void process();


private:
  bool separateLowpassPanning() const { return bool(mLowFrequencyGainOutput); }


  using LoudspeakerIndexType = std::size_t;


  /**
   *
   */
  bool isTwoD() const { return mVectorDimension == 2; }


  /**
  * Set the reference listener position. This overload accepts three cartesian coordinates.
  * The listener positions are used beginning with the next process() call.
  * This method triggers the recalculation of the internal data (e.g., the inverse panning matrices).
  * @param x The x coordinate [in meters]
  * @param y The y coordinate [in meters]
  * @param z The z coordinate [in meters]
  * @throw std::runtime_error If the recalculation of the internal panning data fails.
  * @todo Consider extending the interface to multiple listener positions.
  */
  void setListenerPosition( CoefficientType x, CoefficientType y, CoefficientType z );

  /**
  * Set the reference listener position given as a ListenerPosition object.
  * The listener positions are used beginning with the next process() call.
  * This method triggers the recalculation of the internal data (e.g., the inverse panning matrices).
  * @param pos The listener position.
  * @throw std::runtime_error If the recalculation of the internal panning data fails.
  * @todo Consider extending the interface to multiple listener positions.
  */
  void setListenerPosition( pml::ListenerPosition const & pos );

  /**
   * The number of audio objects handled by this object.
   */
  std::size_t mNumberOfObjects;

  /**
   * The number of panning loudspeakers.
   * @note This excludes any potential subwoofers (which are not handled by the panning algorithm)
   */
  std::size_t mNumberOfRegularLoudspeakers;

  /**
   * Number of all loudspeakers, including virtual (phantom) loudspeakers.
   *
   */
  std::size_t mNumberOfAllLoudspeakers;


  /**
   * The dimension of the loudspeaker and position vectors.
   * Either 3 for a standard 3D panning setup, or 2 for a horizontal-only setup.
   */
  std::size_t mVectorDimension;

  efl::BasicMatrix<CoefficientType> mSourceCoordinates;

  efl::BasicMatrix<CoefficientType> mLoudspeakerPositions;

  efl::BasicMatrix<CoefficientType> mPolygonCenters;

  /**
   * Describe the polygons within the
   */
  //@{
  std::vector<LoudspeakerIndexType> mPolygonIndices;

  std::vector<std::size_t> mPolygonStartOffsets;


  std::size_t numberOfPolygons() const;

  std::size_t tripletStartIndex( std::size_t polygonIdx ) const;

  std::size_t tripletEndIndex( std::size_t polygonIdx ) const;

  std::size_t verticesInPolgon( std::size_t polygonIdx ) const;

  LoudspeakerIndexType polygonVertex( std::size_t polygonIdx, std::size_t vertexIndex ) const;

  LoudspeakerIndexType const & polygonVertices( std::size_t polygonIdx ) const;
  //@}

  efl::BasicVector<CoefficientType> mLoudspeakerDotProducts;

#if 1
  /**
   * The loudspeaker array configuration.
   * @note Because this object must persist for the whole lifetime of the \p mVbapCalculator object,
   * we make a copy of the reference passed to setup method.
   */
  panning::LoudspeakerArray mSpeakerArray;
  
  /**
   * A vector to hold the source position data.
   * @todo: replace this by a variable-sized vector;
   */
  std::vector<panning::XYZ> mSourcePositions;

  /**
   * The calculator object to generate the panning matrix coefficients.
   */
  panning::VBAP mVbapCalculator;
  
  /**
   * The levels of the object channels in linear scale.
   */
  std::valarray<objectmodel::LevelType> mLevels;
  //@}
#endif

  /**
   * Data type of the parmaeter ports for outgoing matrix data.
   */
  using ListenerPositionPort = ParameterInput<pml::DoubleBufferingProtocol, pml::ListenerPosition >;
  using ObjectPort = ParameterInput<pml::DoubleBufferingProtocol, pml::ObjectVector >;
  using MatrixPort = ParameterOutput<pml::SharedDataProtocol, pml::MatrixParameter<CoefficientType> >;

  std::unique_ptr<ObjectPort> mObjectVectorInput;

  std::unique_ptr<ListenerPositionPort> mListenerPositionInput;

  /**
   * Needs to be instantiated as a pointer, because the ParameterConfig data is not known until the setup() method.
   */
  std::unique_ptr<MatrixPort> mGainOutput;

  /**
   * Separate gain output for low-frequency panning.
   */
  std::unique_ptr<MatrixPort> mLowFrequencyGainOutput;

  /**
   * Vector for intermediate results used for separate high/low frequency panning
   */
  efl::BasicVector<CoefficientType> mHighFrequencyGains;
};

} // namespace rcl
} // namespace visr

#endif // #ifndef VISR_LIBRCL_PANNING_CALCULATOR_HPP_INCLUDED
