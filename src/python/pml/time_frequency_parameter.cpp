/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <libpml/time_frequency_parameter.hpp> 
#include <libpml/time_frequency_parameter_config.hpp> 

#include <libefl/basic_matrix.hpp> 

#include <libvisr/constants.hpp>
#include <libvisr/parameter_base.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include <complex>

namespace py = pybind11;

namespace visr
{

using pml::TimeFrequencyParameter;
using pml::TimeFrequencyParameterConfig;

namespace python
{
namespace pml
{

namespace //unnamed
{

template<typename DataType>
void exportTimeFrequencyParameter( py::module & m, char const * className )
{
  using ComplexType = typename TimeFrequencyParameter<DataType>::ComplexType;
  py::class_<TimeFrequencyParameter< DataType >, ParameterBase >(m, className, py::buffer_protocol() )
  .def_property_readonly_static( "staticType", [](py::object /*self*/)
   { return TimeFrequencyParameter< DataType >::staticType(); } )
  .def( py::init<std::size_t>(), py::arg("alignment") = 0 )
  .def( py::init<std::size_t, std::size_t, std::size_t, std::size_t>(), py::arg( "dftSize" ),
    py::arg( "numDftSamples" )=1, py::arg( "numChannels" )=1,
    py::arg( "alignment" )=visr::cVectorAlignmentSamples )
  .def( py::init< TimeFrequencyParameterConfig const &>(), py::arg( "config" ) )
  .def( py::init< visr::ParameterConfigBase const &>(), py::arg( "config" ) )
  .def( py::init( []( py::array_t< ComplexType > const & data, 
    std::size_t alignment)
  {
    if( data.ndim() != 3 )
    {
      throw std::invalid_argument( "TimeFrequencyParameter from numpy ndarray: Input array must be 2D" );
    }
    std::size_t const numDftBlocks = static_cast<py::ssize_t>(data.shape()[0]);
    std::size_t const numChannels = static_cast<py::ssize_t>(data.shape()[1]);
    std::size_t const dftSize = static_cast<py::ssize_t>(data.shape()[2]);
    TimeFrequencyParameter<DataType> * inst = new TimeFrequencyParameter<DataType>( dftSize, numDftBlocks, numChannels, alignment);
    for( std::size_t dftBlockIdx(0); dftBlockIdx < numDftBlocks; ++dftBlockIdx )
    {
      for( std::size_t channelIdx(0); channelIdx < numChannels; ++channelIdx )
      {
        for( std::size_t binIdx(0); binIdx < dftSize; ++binIdx )
        {
          inst->at( dftBlockIdx, channelIdx, binIdx )
            = *static_cast<ComplexType const *>(data.data( dftBlockIdx, channelIdx, binIdx ));
        }
      }
    }
    return inst;
  }), py::arg("data"), py::arg("alignment") = visr::cVectorAlignmentSamples )
  .def_property_readonly( "alignment", &TimeFrequencyParameter<DataType>::alignment )
  .def_property_readonly( "dftSize", &TimeFrequencyParameter<DataType>::dftSize )
  .def_property_readonly( "dftBlocks", &TimeFrequencyParameter<DataType>::dftSamples )
  .def_property_readonly( "numberOfChannels", &TimeFrequencyParameter<DataType>::numberOfChannels )
  .def( "resize", &TimeFrequencyParameter<DataType>::resize, py::arg( "dftSize" ), py::arg( "numDftBlocks" )=1,
    py::arg( "numChannels" )=1 )
  .def( "__getitem__", []( TimeFrequencyParameter<DataType> const & self, py::tuple idx ) { return self.at( idx[0].cast<std::size_t>(), idx[1].cast<std::size_t>(), idx[2].cast<std::size_t>() ); }, py::arg( "index" ) )
  .def( "__setitem__", []( TimeFrequencyParameter<DataType> & self, py::tuple idx, ComplexType val ) { self.at( idx[0].cast<std::size_t>(), idx[1].cast<std::size_t>(), idx[2].cast<std::size_t>() ) = val; }, py::arg( "index" ), py::arg( "value" ) )
  .def_buffer([](TimeFrequencyParameter<DataType> &tfp) -> py::buffer_info
                {
                  return py::buffer_info( tfp.data(),
                                                sizeof( ComplexType ),
                                                py::format_descriptor<ComplexType>::format(),
                                                3,
                                                { tfp.dftSamples(), tfp.numberOfChannels(), tfp.dftSize() },
                                                { sizeof( ComplexType ) * tfp.dftSampleStride(), sizeof( ComplexType ) *  tfp.channelStride(), sizeof( ComplexType ) } );
                }
      )
  ;
}

} // unnamed namespace

void exportTimeFrequencyParameter( py::module & m)
{
  py::class_< TimeFrequencyParameterConfig, ParameterConfigBase >(
      m, "TimeFrequencyParameterConfig" )
      .def( py::init< std::size_t, std::size_t, std::size_t >(),
            py::arg( "numberOfDftBins" ), py::arg( "numberOfChannels" ) = 1,
            py::arg( "numberOfFrames" ) = 1 )
      .def_property_readonly( "dftSize",
                              &TimeFrequencyParameterConfig::numberOfDftBins )
      .def_property_readonly( "numberOfChannels",
                              &TimeFrequencyParameterConfig::numberOfChannels )
      .def_property_readonly( "numberOfFrames",
                              &TimeFrequencyParameterConfig::numberOfFrames )
      .def( "compare",
            static_cast< bool ( TimeFrequencyParameterConfig::* )(
                TimeFrequencyParameterConfig const & ) const >(
                &TimeFrequencyParameterConfig::compare ),
            py::arg( "rhs" ) )
      .def( "compare",
            static_cast< bool ( TimeFrequencyParameterConfig::* )(
                ParameterConfigBase const & ) const >(
                &TimeFrequencyParameterConfig::compare ),
            py::arg( "rhs" ) );

  exportTimeFrequencyParameter< float >( m, "TimeFrequencyParameterFloat" );
  exportTimeFrequencyParameter< double >( m, "TimeFrequencyParameterDouble" );
}

} // namespace pml
} // namespace python
} // namespace visr
