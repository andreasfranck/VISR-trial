/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "diffusion_gain_calculator.hpp"

#include <libefl/basic_matrix.hpp>
#include <libefl/basic_vector.hpp>

#include <libobjectmodel/object_vector.hpp>
// Note: At the moment, all supported source types are translated directly in this file
// TODO: For the future, consider moving this to another location.
#include <libobjectmodel/diffuse_source.hpp>
#include <libobjectmodel/point_source_with_diffuseness.hpp>


#include <ciso646>
#include <iostream>

namespace visr
{
namespace rcl
{

  DiffusionGainCalculator::DiffusionGainCalculator( SignalFlowContext const & context,
                                                    char const * name,
                                                    CompositeComponent * parent,
                                                    std::size_t numberOfObjectChannels )
  : AtomicComponent( context, name, parent )
  , mNumberOfObjectChannels( numberOfObjectChannels )
  , mObjectVectorInput( "objectInput", *this, pml::EmptyParameterConfig() )
  , mGainOutput( "gainOutput", *this, pml::MatrixParameterConfig( 1, numberOfObjectChannels ) )
{
}

DiffusionGainCalculator::~DiffusionGainCalculator()
{
}

void DiffusionGainCalculator::process()
{
  objectmodel::ObjectVector const & objects = mObjectVectorInput.data();
  pml::MatrixParameter<CoefficientType> & gainMatrix = mGainOutput.data();
  if( (gainMatrix.numberOfRows() != 1) or (gainMatrix.numberOfColumns() != mNumberOfObjectChannels) )
  {
    throw std::invalid_argument( "DiffusionGainCalculator::process(): The gainVector argument must be a vector with numberOfObjectChannels elements." );
  }
  gainMatrix.zeroFill( );
  processInternal( objects, gainMatrix.row( 0 ) );
}

void DiffusionGainCalculator::processInternal( objectmodel::ObjectVector const & objects, CoefficientType * gains )
{
  // For the moment, we assume that the audio channels of the objects are identical to the final channel numbers.
  // Any potential re-routing will be added later.
  for( objectmodel::Object const & obj : objects )
  {
    // Pre-check to handle only monaural objects here. The fine-grained disambiguation between supported object types happens later.
    if( obj.numberOfChannels() != 1 )
    {
      continue;
    }
    objectmodel::Object::ChannelIndex const channelId = obj.channelIndex( 0 );
    if( channelId >= mNumberOfObjectChannels )
    {
      std::cerr << "DiffusionGainCalculator: Channel index \"" << channelId << "\" of object id#" << obj.id()
                << "exceeds number of channels (" << mNumberOfObjectChannels << ")." << std::endl;
      continue;
    }

    objectmodel::ObjectTypeId const ti = obj.type();

    // For the moment, we treat the two supported source type here.
    // @todo find a proper abstraction to handle many source types.
    switch( ti )
    {
    case objectmodel::ObjectTypeId::DiffuseSource:
    {
      gains[channelId] = 1.0f;
      break;
    }
    case objectmodel::ObjectTypeId::PointSourceWithDiffuseness:
    {
      objectmodel::PointSourceWithDiffuseness const & psdSrc = dynamic_cast<objectmodel::PointSourceWithDiffuseness const &>(obj);
      gains[channelId] = psdSrc.diffuseness();
      break;
    }
    default:
    // Other source types, including hitherto unknown, are set to zero diffuseness.
    gains[channelId] = static_cast<objectmodel::LevelType>(0.0f);
    }
  } // for( objectmodel::ObjectVector::value_type const & objEntry : objects )
}

} // namespace rcl
} // namespace visr
