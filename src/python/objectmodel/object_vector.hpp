/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_PYTHON_VISR_OBJECTMODEL_OBJECT_VECTOR_HPP_INCLUDED
#define VISR_PYTHON_VISR_OBJECTMODEL_OBJECT_VECTOR_HPP_INCLUDED

#include <pybind11/pybind11.h>


namespace visr
{
namespace objectmodel
{
namespace python
{

void exportObjectVector( pybind11::module& m );

} // namespace python
} // namespace objectmodel
} // namespace visr

#endif // #ifndef VISR_PYTHON_VISR_OBJECTMODEL_OBJECT_VECTOR_HPP_INCLUDED
