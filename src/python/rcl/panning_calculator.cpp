/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <librcl/panning_calculator.hpp>

#include <libvisr/atomic_component.hpp>
#include <libvisr/composite_component.hpp>
#include <libvisr/signal_flow_context.hpp>

#include <libpanning/LoudspeakerArray.h>

#include <pybind11/pybind11.h>

namespace visr
{
namespace python
{
namespace rcl
{

void exportPanningCalculator( pybind11::module & m )
{
  using visr::rcl::PanningCalculator;

  pybind11::class_<PanningCalculator, visr::AtomicComponent>( m, "PanningCalculator" )
   .def( pybind11::init<visr::SignalFlowContext const&, char const *, visr::CompositeComponent*,
     std::size_t, panning::LoudspeakerArray const &, bool, bool>(),
      pybind11::arg("context"), pybind11::arg("name"), pybind11::arg("parent") = static_cast<visr::CompositeComponent*>(nullptr),
      pybind11::arg( "numberOfObjects" ),
      pybind11::arg( "arrayConfig" ), pybind11::arg( "adaptiveListener" ) = false,
      pybind11::arg( "separateLowpassPanning" ) = false )
  ; 
}

} // namepace rcl
} // namespace python
} // namespace visr
