/* Copyright Institute of Sound and Vibration Research - All rights reserved */


#include <pybind11/pybind11.h>

// Forward declarations
namespace visr
{
namespace rbbl
{
namespace python
{
  void exportCoreConvolversUniform( pybind11::module & m );
  void exportCrossfadingConvolversUniform( pybind11::module & m );
  void exportFilterRouting( pybind11::module & m );
  void exportInterpolationParameter( pybind11::module & m );
  void exportMultichannelConvolversUniform( pybind11::module & m );
  void exportObjectChannelAllocator( pybind11::module & m );
}
}
}

PYBIND11_MODULE( rbbl, m )
{
  using namespace visr::rbbl::python;
  exportFilterRouting( m ); // Needs to come before the convolvers
  exportInterpolationParameter( m );
  exportCoreConvolversUniform( m );
  exportCrossfadingConvolversUniform( m );
  exportMultichannelConvolversUniform( m );
  exportObjectChannelAllocator( m );
}
