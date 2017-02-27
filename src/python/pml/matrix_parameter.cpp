/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <libpml/matrix_parameter.hpp> 
#include <libpml/matrix_parameter_config.hpp> 

#include <libril/constants.hpp>

#ifdef USE_PYBIND11
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#else
#include <boost/python.hpp>
#endif


namespace visr
{

using pml::MatrixParameter;
using pml::MatrixParameterConfig;

namespace python
{
namespace pml
{

#ifdef USE_PYBIND11

template<typename DataType>
void exportMatrixParameter( pybind11::module & m, char const * className )
{
  pybind11::class_<MatrixParameter< DataType >/*, ParameterBase*/ >(m, className, pybind11::metaclass(), pybind11::buffer_protocol() )
  .def_buffer([](MatrixParameter<DataType> &mp) -> pybind11::buffer_info
  {
    return pybind11::buffer_info( mp.data(),
     sizeof( DataType ),
     pybind11::format_descriptor<DataType>::format(),
     2,
     { mp.numberOfRows(), mp.numberOfColumns() },
     { sizeof( DataType ), mp.stride() * sizeof( DataType ) } );
  } )
  .def( pybind11::init<std::size_t>(), pybind11::arg("alignment") = visr::cVectorAlignmentSamples )
  .def( pybind11::init<std::size_t, std::size_t, std::size_t>() )
  .def( "__init__", []( MatrixParameter<DataType> & inst, pybind11::array const & data, std::size_t alignment)
  {
    if( data.ndim() != 2 )
    {
      throw std::invalid_argument( "MatrixParameter from numpy ndarray: Input aray must be 2D" );
    }
    std::size_t const numRows = data.shape()[0];
    std::size_t const numCols = data.shape()[1];
    new (&inst) MatrixParameter<DataType>( numRows, numCols, alignment);
    for( std::size_t rowIdx(0); rowIdx < numRows; ++rowIdx )
    {
      for( std::size_t colIdx(0); colIdx < numCols; ++colIdx )
      {
        inst( rowIdx, colIdx ) = *static_cast<DataType const *>(data.data( rowIdx, colIdx ));
      }
    }
  }, pybind11::arg("data"), pybind11::arg("alignment") = visr::cVectorAlignmentSamples )
  .def_property_readonly( "numberOfRows", &MatrixParameter<DataType>::numberOfRows )
  .def_property_readonly( "numberOfColumns", &MatrixParameter<DataType>::numberOfColumns )
  .def( "resize", &MatrixParameter<DataType>::resize, pybind11::arg("numberOfRows"), pybind11::arg("numberOfColumns") )
  .def( "zeroFill", &MatrixParameter<DataType>::zeroFill )
    .def_static( "fromAudioFile", &MatrixParameter<DataType>::fromAudioFile, pybind11::arg("file"), pybind11::arg("alignment") = visr::cVectorAlignmentSamples ) 

  ;
}

void exportMatrixParameters( pybind11::module & m)
{
  pybind11::class_<MatrixParameterConfig, ParameterConfigBase >( m, "MatrixParameterConfig" )
    .def( pybind11::init<std::size_t, std::size_t>(), pybind11::arg("numberOfRows" ), pybind11::arg("numberOfColumns") )
    .def_property_readonly( "numberOfRows", &MatrixParameterConfig::numberOfRows )
    .def_property_readonly( "numberOfColumns", &MatrixParameterConfig::numberOfColumns )
    .def( "compare", static_cast<bool(MatrixParameterConfig::*)(MatrixParameterConfig const&) const>(&MatrixParameterConfig::compare),  pybind11::arg("rhs") )
    .def( "compare", static_cast<bool(MatrixParameterConfig::*)(ParameterConfigBase const&) const>(&MatrixParameterConfig::compare),  pybind11::arg("rhs") )
  ;

  exportMatrixParameter<float>( m, "MatrixParameterFloat" );
  exportMatrixParameter<double>( m, "MatrixParameterDouble" );
}

#else
using namespace boost::python;

template<typename DataType>
void exportMatrixParameter( char const * className )
{
  boost::python::class_<MatrixParameter< DataType > >( className, init<std::size_t>( args("alignment") ) )
   .def( init<std::size_t, std::size_t, std::size_t>() )
   .add_property( "numberOfRows", &MatrixParameter<DataType>::numberOfRows )
   .add_property( "numberOfColumns", &MatrixParameter<DataType>::numberOfColumns )
   .def( "resize", &MatrixParameter<DataType>::resize, (arg("numberOfRows"), arg("numberOfColumns") ) )
   .def( "zeroFill", &MatrixParameter<DataType>::zeroFill )
    .def_static( "fromAudioFile", &MatrixParameter<DataType>::fromAudioFile, pybind11::arg("file"), pybind11::arg("alignment") = SampleType ) 
    ;
}
  
void exportMatrixParameters()
{
  exportMatrixParameter<float>( "MatrixParameterFloat" );
  exportMatrixParameter<double>( "MatrixParameterDouble" );
}
#endif
} // namepace pml
} // namespace python
} // namespace visr
