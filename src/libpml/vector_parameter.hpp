/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_PML_VECTOR_PARAMETER_HPP_INCLUDED
#define VISR_PML_VECTOR_PARAMETER_HPP_INCLUDED

#include "vector_parameter_config.hpp"

#include <libefl/basic_vector.hpp>

#include <libril/parameter_type.hpp>
#include <libril/typed_parameter_base.hpp>

#include <complex>

namespace visr
{
namespace pml
{

namespace // unnamed
{
/**
 * Type trait to assign a unique type id to each concrete VectorParameter template instantiation.
 */
template<typename ElementType> struct VectorParameterType {};

template<> struct VectorParameterType<float>
{ static constexpr const ParameterType ptype() { return detail::compileTimeHashFNV1( "FloatVector" ); } };

template<> struct VectorParameterType<double>
{ static constexpr const ParameterType ptype() { return detail::compileTimeHashFNV1( "DoubleVector" ); } };

template<> struct VectorParameterType<std::complex<float> >
{ static constexpr const ParameterType ptype() { return detail::compileTimeHashFNV1( "ComplexFloatVector" ); } };

template<> struct VectorParameterType<std::complex<double> >
{ static constexpr const ParameterType ptype() { return detail::compileTimeHashFNV1( "ComplexDoubleVector" ); } };
} // unnamed namespace

/**
 * A type for passing vectors between processing components.
 * The template class is explicitly instantiated for the element types float and double.
 * @tparam ElementType The data type of the elements of the matrix.
 */
template<typename ElementType >
class VectorParameter: public efl::BasicVector<ElementType>,
  public TypedParameterBase<VectorParameterConfig, VectorParameterType<ElementType>::ptype() >
{
public:
  explicit VectorParameter(ParameterConfigBase const & config);

  explicit VectorParameter(VectorParameterConfig const & config);
};

} // namespace pml
} // namespace visr

DEFINE_PARAMETER_TYPE( visr::pml::VectorParameter<float>, visr::pml::VectorParameter<float>::staticType(), visr::pml::VectorParameterConfig )
DEFINE_PARAMETER_TYPE( visr::pml::VectorParameter<double>, visr::pml::VectorParameter<double>::staticType(), visr::pml::VectorParameterConfig )
DEFINE_PARAMETER_TYPE( visr::pml::VectorParameter< std::complex<float> >, visr::pml::VectorParameter< std::complex<float> >::staticType(), visr::pml::VectorParameterConfig )
DEFINE_PARAMETER_TYPE( visr::pml::VectorParameter< std::complex<double> >, visr::pml::VectorParameter< std::complex<double> >::staticType(), visr::pml::VectorParameterConfig )

#endif // VISR_PML_VECTOR_PARAMETER_HPP_INCLUDED
