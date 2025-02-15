/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_POLYMORPHIC_PARAMETER_INPUT_HPP_INCLUDED
#define VISR_POLYMORPHIC_PARAMETER_INPUT_HPP_INCLUDED

#include "export_symbols.hpp"
#include "parameter_input.hpp"
#include "communication_protocol_base.hpp"

#include <memory>

namespace visr
{

/**
 * Parameter input port without predefined parameter and port types.
 * In contrast to the templated ParameterInput classes, these types are set by parameter and protocol type ids 
 * that are passed as constructor arguments.
 */
class VISR_CORE_LIBRARY_SYMBOL PolymorphicParameterInput: public ParameterInputBase
{
public:
  /**
   * Constructor with a parameter config argument.
   * @param name The name of the port, must be unique within the parameter ports of the containing components.
   * @param parent the containing component (atomic or composite)
   * @param parameterType The parameter type id for this port.
   * @param protocolType The protocol type id for this port.
   * @param paramConfig A parameter configuration object. Must be compatible with the parameter type specified by \p parameterType.
   */
  /*VISR_CORE_LIBRARY_SYMBOL*/ explicit PolymorphicParameterInput( char const * name,
                                                               Component & parent,
                                                               ParameterType const & parameterType,
                                                               CommunicationProtocolType const & protocolType,
                                                               ParameterConfigBase const & paramConfig );

  /**
  * Constructor without a parameter config argument. The parameter type must be set during the initalisation set.
  * @param name The name of the port, must be unique within the parameter ports of the containing components.
  * @param parent the containing component (atomic or composite)
  * @param parameterType The parameter type id for this port.
  * @param protocolType The protocol type id for this port.
  */
  /*VISR_CORE_LIBRARY_SYMBOL*/ explicit PolymorphicParameterInput( char const * name,
                                                               Component & parent,
                                                               ParameterType const & parameterType,
                                                               CommunicationProtocolType const & protocolType );

  /**
   * Virtual destructor.
   * Overrides the base class destructor (of ParameterInputBase).
   */
  /*VISR_CORE_LIBRARY_SYMBOL*/ virtual ~PolymorphicParameterInput() override;

  /**
   * Set the communication protocol for the protocol input contained in this port.
   * Defines the pure virtual method of the parameterInputBase protocol.
   */
  /*VISR_CORE_LIBRARY_SYMBOL*/ void setProtocol( CommunicationProtocolBase * protocol ) override;

  /**
   * Return the protocol input of this port.
   * Must be called only after initialisation.
   * Defines the pure virtual method of the ParameterInputBase protocol.
   * @throw std::exception If the method is called while this object is not connected.
   */
  /*VISR_CORE_LIBRARY_SYMBOL*/ CommunicationProtocolBase::Input & protocolInput() override;

  /**
   * Return the protocol input of this port, const version
   * Must be called only after initialisation.
   * Defines the pure virtual method of the ParameterInputBase protocol.
   * @throw std::exception If the method is called while this object is not connected.
   */
  /*VISR_CORE_LIBRARY_SYMBOL*/ CommunicationProtocolBase::Input const & protocolInput() const;

private:
  /**
   * The communication protocol input matching the concrete protocol type, instantiated dynamically.
   */
  std::unique_ptr<CommunicationProtocolBase::Input> mProtocolInput;
};

} // namespace visr

#endif // #ifndef VISR_POLYMORPHIC_PARAMETER_INPUT_HPP_INCLUDED
