/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_PML_OBJECT_VECTOR_HPP_INCLUDED
#define VISR_PML_OBJECT_VECTOR_HPP_INCLUDED

#include "empty_parameter_config.hpp"

#include <libobjectmodel/object_vector.hpp>

#include <libril/parameter_type.hpp>
#include <libril/typed_parameter_base.hpp>


#include <cstdint>
#include <iosfwd>
#include <istream>

namespace visr
{
namespace pml
{

class ObjectVector: public objectmodel::ObjectVector,
                    public TypedParameterBase<EmptyParameterConfig, ParameterType::ObjectVector >
{
public:
  explicit ObjectVector( ParameterConfigBase const & config );

  explicit ObjectVector( EmptyParameterConfig const & config );

  /**
   * Copy constructor.
   * In contrast to the base class objectmodel::ObjectVector, we allow copy construction to enable use of objectVectors in communication protocols which require copying.
   */
  ObjectVector( const ObjectVector & rhs );

  /**
   * Assignment operator.
   */
  ObjectVector & operator=(const ObjectVector & rhs);

  virtual ~ObjectVector() override;
};

} // namespace pml
} // namespace visr

DEFINE_PARAMETER_TYPE( visr::pml::ObjectVector, visr::ParameterType::ObjectVector, visr::pml::EmptyParameterConfig )

#endif // VISR_PML_OBJECT_VECTOR_HPP_INCLUDED
