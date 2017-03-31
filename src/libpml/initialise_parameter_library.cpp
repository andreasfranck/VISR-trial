/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "initialise_parameter_library.hpp"

#include "double_buffering_protocol.hpp"
#include "message_queue_protocol.hpp"
#include "shared_data_protocol.hpp"

#include "biquad_parameter.hpp"
#include "indexed_value_parameter.hpp"
#include "matrix_parameter.hpp"
#include "listener_position.hpp"
#include "object_vector.hpp"
#include "string_parameter.hpp"
#include "signal_routing_parameter.hpp"
#include "time_frequency_parameter.hpp"
#include "vector_parameter.hpp"

#include <libril/parameter_factory.hpp>
#include <libril/communication_protocol_factory.hpp>

namespace visr
{
namespace pml
{

void initialiseParameterLibrary()
{
  
  //CommunicationProtocolFactory::registerCommunicationProtocol< DoubleBufferingProtocol >( DoubleBufferingProtocol::staticType(), DoubleBufferingProtocol::staticName() );
  //CommunicationProtocolFactory::registerCommunicationProtocol< MessageQueueProtocol >( MessageQueueProtocol::staticType(), DoubleBufferingProtocol::staticName() );
  //CommunicationProtocolFactory::registerCommunicationProtocol< SharedDataProtocol >( SharedDataProtocol::staticType(), DoubleBufferingProtocol::staticName() );

  CommunicationProtocolFactory::registerCommunicationProtocol< DoubleBufferingProtocol >();
  CommunicationProtocolFactory::registerCommunicationProtocol< MessageQueueProtocol >();
  CommunicationProtocolFactory::registerCommunicationProtocol< SharedDataProtocol >();

  // Register all supported parameter types.
  ParameterFactory::registerParameterType< BiquadParameterMatrix<float> >( BiquadParameterMatrix<float>::staticType() );
  ParameterFactory::registerParameterType< BiquadParameterMatrix<double> >( BiquadParameterMatrix<double>::staticType() );

  ParameterFactory::registerParameterType< IndexedVectorDoubleType >( IndexedVectorDoubleType::staticType() );
  ParameterFactory::registerParameterType< IndexedVectorFloatType >( IndexedVectorFloatType::staticType() );
  ParameterFactory::registerParameterType< IndexedStringType >( IndexedStringType::staticType() );

  ParameterFactory::registerParameterType< ListenerPosition >( ListenerPosition::staticType() );

  ParameterFactory::registerParameterType< MatrixParameter<float> >( MatrixParameter<float>::staticType() );
  ParameterFactory::registerParameterType< MatrixParameter<double> >( MatrixParameter<double>::staticType() );
  ParameterFactory::registerParameterType< MatrixParameter<std::complex<float> > >( MatrixParameter<std::complex<float> >::staticType() );
  ParameterFactory::registerParameterType< MatrixParameter<std::complex<double> > >( MatrixParameter<std::complex<double> >::staticType() );

  ParameterFactory::registerParameterType< ObjectVector >(ObjectVector::staticType() );

  ParameterFactory::registerParameterType< ListenerPosition >( );

  ParameterFactory::registerParameterType< SignalRoutingParameter >();

  ParameterFactory::registerParameterType< StringParameter >();

  ParameterFactory::registerParameterType< TimeFrequencyParameter<float> >();
  ParameterFactory::registerParameterType< TimeFrequencyParameter<double> >();

  ParameterFactory::registerParameterType< VectorParameter<float> >();
  ParameterFactory::registerParameterType< VectorParameter<double> >();
}

} // namespace pml
} // namespace visr
