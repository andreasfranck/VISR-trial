/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <libpanning/LoudspeakerArray.h>
#include <libpml/biquad_parameter.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <vector>

namespace visr
{
using panning::LoudspeakerArray;

namespace python
{
namespace panning
{

namespace py = pybind11;

namespace
{

/** Internal function to create the array of channel indices */
std::vector<LoudspeakerArray::ChannelIndex> getChannelIndices( LoudspeakerArray const & array)
{
  std::size_t numCh = array.getNumRegularSpeakers();
  if( numCh == 0 )
  {
    return std::vector<LoudspeakerArray::ChannelIndex>();
  }
  else
  {
    return std::vector<LoudspeakerArray::ChannelIndex>(
      array.getLoudspeakerChannels(), array.getLoudspeakerChannels() + numCh );
  }
}

/** 
 * Local function to transform the subwoofer index list into std::vector
 */
std::vector<LoudspeakerArray::ChannelIndex> getSubwooferChannelIndices( LoudspeakerArray const & array )
{

  std::size_t const numSubs = array.getNumSubwoofers();
  if( numSubs == 0 )
  {
    return std::vector<LoudspeakerArray::ChannelIndex>();
  }
  else
  {
    return std::vector<LoudspeakerArray::ChannelIndex>(
      array.getSubwooferChannels(), array.getSubwooferChannels() + numSubs );
  }
}

py::array getPositions( LoudspeakerArray const & la )
{
  std::size_t const len = la.getNumSpeakers();
  py::array ret( py::dtype::of<float>(),
                 py::array::ShapeContainer( {static_cast<long int>(len), 3 }), // parameter needs to be signed. pybind11 uses ssize_t, which is not in the C/C++ standard. 
  { sizeof( Afloat )*3, sizeof( Afloat ) }
  );
  for( std::size_t lspIdx(0); lspIdx < len; ++lspIdx )
  {
    visr::panning::XYZ const pos = la.getPosition(lspIdx);

    *(static_cast<Afloat*>(ret.mutable_data( lspIdx, 0 ))) = pos.x;
    *(static_cast<Afloat*>(ret.mutable_data( lspIdx, 1 ))) = pos.y;
    *(static_cast<Afloat*>(ret.mutable_data( lspIdx, 2 ))) = pos.z;
  }
  return ret;
}

} // namespace unnamed

void exportLoudspeakerArray( pybind11::module & m)
{
  pybind11::class_<LoudspeakerArray>(m, "LoudspeakerArray")
    .def(pybind11::init<>(), "Default constructor" )
    .def(pybind11::init<std::string const &>(), pybind11::arg("xmlFile"), "Constructor from a file name")
    .def("loadXmlFile", &LoudspeakerArray::loadXmlFile)
    .def( "loadXmlString", &LoudspeakerArray::loadXmlString)
    //.def("getPosition", (visr::panning::XYZ &(LoudspeakerArray::*)(size_t iSpk)) &LoudspeakerArray::getPosition)
    //.def("getPosition", (visr::panning::XYZ const &(LoudspeakerArray::*)(size_t iSpk) const) &LoudspeakerArray::getPosition)
    //.def( "getPosition", (visr::panning::XYZ &(LoudspeakerArray::*)(LoudspeakerArray::LoudspeakerIdType const & iSpk)) &LoudspeakerArray::getPosition )
    .def( "position", (visr::panning::XYZ const &(LoudspeakerArray::*)(LoudspeakerArray::LoudspeakerIdType const &) const)(&LoudspeakerArray::getPosition) )
    .def( "positions", &getPositions, "Return the loudspeaker positions (real and imaginary) as a Numpy array." )
    .def( "getSpeakerChannelIndex", &LoudspeakerArray::channelIndex, pybind11::arg("speakerIndex"))
    .def( "getSpeakerIndexFromId", &LoudspeakerArray::getSpeakerIndexFromId, pybind11::arg( "speakerId" ) )
    .def( "getSpeakerChannel", &LoudspeakerArray::getSpeakerChannel, pybind11::arg( "speakerIndex" ) )
    .def( "getSpeakerChannelFromId", &LoudspeakerArray::getSpeakerChannelFromId, pybind11::arg( "speakerId" ) )
    .def( "setTriplet", &LoudspeakerArray::setTriplet, pybind11::arg("tripletIndex"), pybind11::arg("speakerId1"), pybind11::arg("speakerId2"), pybind11::arg("speakerId3"))
    .def( "getTriplet", (LoudspeakerArray::TripletType &(LoudspeakerArray::*)(size_t iTri))&LoudspeakerArray::getTriplet)
    .def( "getTriplet", (LoudspeakerArray::TripletType const &(LoudspeakerArray::*)(size_t iTri) const) &LoudspeakerArray::getTriplet)
    .def_property_readonly("totNumberOfLoudspeakers", &LoudspeakerArray::getNumSpeakers)
    .def_property_readonly("numberOfRegularLoudspeakers", &LoudspeakerArray::getNumRegularSpeakers)
    .def_property_readonly("numberOfTriplets", &LoudspeakerArray::getNumTriplets)
    .def_property_readonly("is2D", &LoudspeakerArray::is2D)
    .def_property_readonly("isInfinite", &LoudspeakerArray::isInfinite)
    .def_property_readonly("numberOfSubwoofers", &LoudspeakerArray::getNumSubwoofers)
    .def("subwooferChannelIndices", &getSubwooferChannelIndices )
    .def("subwooferChannelIndex", &LoudspeakerArray::getSubwooferChannel, pybind11::arg("subwooferIndex"))
    .def("getSubwooferGains", &LoudspeakerArray::getSubwooferGains, pybind11::return_value_policy::reference)
    .def("getReRoutingCoefficients", &LoudspeakerArray::getReroutingCoefficients, pybind11::return_value_policy::reference)
    .def("getReRoutingCoefficient", &LoudspeakerArray::getReroutingCoefficient, pybind11::arg("virtualSpeakerIdx"), pybind11::arg("realSpeakerIdx"))
    .def("getLoudspeakerGainAdjustment", &LoudspeakerArray::getLoudspeakerGainAdjustment, pybind11::arg("speakerIdx"))
    .def("getLoudspeakerDelayAdjustment", &LoudspeakerArray::getLoudspeakerDelayAdjustment, pybind11::arg("speakerIdx"))
    .def("getSubwooferGainAdjustment", &LoudspeakerArray::getSubwooferGainAdjustment, pybind11::arg("speakerIdx"))
    .def("getSubwooferDelayAdjustment", &LoudspeakerArray::getSubwooferDelayAdjustment, pybind11::arg("speakerIdx"))
    .def( "getGainAdjustments", []( LoudspeakerArray const & la ) { return std::vector<SampleType>( la.getGainAdjustment().data(), la.getGainAdjustment().data()+la.getGainAdjustment().size()); } )
    .def( "getDelayAdjustments", []( LoudspeakerArray const & la ) { return std::vector<SampleType>( la.getDelayAdjustment().data(), la.getDelayAdjustment().data() + la.getDelayAdjustment().size() ); } )
    .def("isOutputEqualisationPresent", &LoudspeakerArray::outputEqualisationPresent)
    .def("outputEqualisationNumberOfBiquads", &LoudspeakerArray::outputEqualisationNumberOfBiquads)
    .def("outputEqualisationBiquads", &LoudspeakerArray::outputEqualisationBiquads)
    .def( "channelIndices", &getChannelIndices )
    ;
}
} // namepace panning
} // namespace python
} // namespace visr
