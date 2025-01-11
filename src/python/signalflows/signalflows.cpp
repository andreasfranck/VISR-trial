/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <pybind11/pybind11.h>

namespace visr
{
namespace python
{
namespace signalflows
{
void exportBaselineRenderer( pybind11::module& m );
void exportCoreRenderer( pybind11::module& m );
void exportDelayVector( pybind11::module& m );
#ifdef VISR_RENDERER_PYTHON_BINDING
  void exportVisrRenderer( pybind11::module& m );
#endif
}
}
}

PYBIND11_MODULE( signalflows, m )
{
  pybind11::module::import( "visr" );
  pybind11::module::import( "visr.pml" );
  pybind11::module::import( "visr.rcl" );
//  pybind11::module::import( "visr.panning" );

  using namespace visr::python::signalflows;
  exportBaselineRenderer( m );
  exportCoreRenderer( m );
  exportDelayVector( m );
#ifdef VISR_RENDERER_PYTHON_BINDING
  exportVisrRenderer( m );
#pragma message( "BUILD VISR Renderer" )
#endif
}
