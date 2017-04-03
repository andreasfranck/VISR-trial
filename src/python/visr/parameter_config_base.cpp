/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "parameter_config_base.hpp"

#include <libril/parameter_config_base.hpp>


#ifdef USE_PYBIND11
#include <pybind11/pybind11.h>
#else
#include <boost/noncopyable.hpp>
#include <boost/python.hpp>
#include <boost/python/args.hpp>
#endif


#include <ciso646>
#include <iostream> // For debugging purposes only.


namespace visr
{
namespace python
{
namespace visr
{

#ifdef USE_PYBIND11

/**
 * Wrapper class to get access to the full functionality
 * Apparently nor required anymore (and is troublesome when deducing the argument
 * type).
 */
class ParameterConfigBaseWrapper: public ParameterConfigBase
{
public:
  /**
   * Use base class constructors
   */
  using ParameterConfigBase::ParameterConfigBase;


  bool compare( ParameterConfigBase const & rhs ) const override
  {
    PYBIND11_OVERLOAD_PURE( bool, ParameterConfigBase, compare, rhs );
  }

  // Pybind11 doesn't find a correct cast for std::unique_ptr return type.
  // TODO: Resolve this problem!
  //std::unique_ptr<ParameterConfigBase> clone() const override
  //{
  //  PYBIND11_OVERLOAD_PURE( std::unique_ptr<ParameterConfigBase>, ParameterConfigBase, clone );
  //}


};

void exportParameterConfigBase( pybind11::module& m )
{
  pybind11::class_<ParameterConfigBase, ParameterConfigBaseWrapper>(m, "ParameterConfigBase" )
    .def( "compare", &ParameterConfigBase::compare )
//    .def( "clone", &ParameterConfigBase::clone )
    ;
}

#else

#error "ParameterConfigBase binding not implemented for boost::python"

#endif

} // namepace visr
} // namespace python
} // namespace visr

