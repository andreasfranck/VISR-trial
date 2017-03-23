/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <libril/parameter_port_base.hpp>

#include <libril/component.hpp>

#include <libvisr_impl/parameter_port_base_implementation.hpp>

namespace visr
{

ParameterPortBase::ParameterPortBase( std::string const & name,
                                      Component & parent, 
                                      PortBase::Direction direction )
  : mImpl( new impl::ParameterPortBaseImplementation( name, *this, &(parent.implementation()), direction ) )
{
}

ParameterPortBase::~ParameterPortBase( ) 
{
  if( mImpl )
  {
    delete mImpl;
    mImpl = nullptr;
  }
}

ParameterType ParameterPortBase::parameterType() const
{
  return mImpl->parameterType();
}

CommunicationProtocolType ParameterPortBase::protocolType() const
{
  return mImpl->protocolType();
}

ParameterConfigBase const & ParameterPortBase::parameterConfig() const
{
  return mImpl->parameterConfig();
}

impl::ParameterPortBaseImplementation & ParameterPortBase::implementation()
{
  return *mImpl;
}

impl::ParameterPortBaseImplementation const & ParameterPortBase::implementation() const
{
  return *mImpl;
}

} // namespace visr
