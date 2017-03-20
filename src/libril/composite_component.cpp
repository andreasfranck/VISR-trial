/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "composite_component.hpp"

#include <libvisr_impl/composite_component_implementation.hpp>

namespace visr
{

CompositeComponent::CompositeComponent( SignalFlowContext& context,
                                        char const * name,
                                         CompositeComponent * parent /*= nullptr*/ )
 : Component( std::unique_ptr<impl::CompositeComponentImplementation>(new impl::CompositeComponentImplementation( *this, context, name,
  (parent == nullptr) ? nullptr : &(parent->implementation()) )) )
{
}

CompositeComponent::~CompositeComponent()
{
}

std::size_t CompositeComponent::numberOfComponents() const
{
  return implementation().numberOfComponents();
}

void CompositeComponent::registerParameterConnection( std::string const & sendComponent,
                                                      std::string const & sendPort,
                                                      std::string const & receiveComponent,
                                                      std::string const & receivePort )
{
  implementation().registerParameterConnection( sendComponent, sendPort, receiveComponent, receivePort );
}

void CompositeComponent::registerParameterConnection( ParameterPortBase & sender,
                                                      ParameterPortBase & receiver )
{
  implementation().registerParameterConnection( sender, receiver );
}

void CompositeComponent::audioConnection( std::string const & sendComponent,
					  std::string const & sendPort,
					  ChannelList const & sendIndices,
					  std::string const & receiveComponent,
					  std::string const & receivePort,
					  ChannelList const & receiveIndices )
{
  implementation().audioConnection( sendComponent, sendPort, sendIndices,
				    receiveComponent, receivePort, receiveIndices );
}

void CompositeComponent::audioConnection( AudioPortBase & sendPort,
					  ChannelList const & sendIndices,
					  AudioPortBase & receivePort,
					  ChannelList const & receiveIndices )
{
  implementation().audioConnection( sendPort, sendIndices, receivePort, receiveIndices );
}


void CompositeComponent::audioConnection( AudioPortBase & sendPort,
                                                  AudioPortBase & receivePort )
{
  implementation().audioConnection( sendPort, receivePort );
}

impl::CompositeComponentImplementation & CompositeComponent::implementation()
{
  // Cast is safe since the constructor ensures that the impl object is of the derived type.
  return static_cast<impl::CompositeComponentImplementation &>(Component::implementation());
}

impl::CompositeComponentImplementation const & CompositeComponent::implementation() const
{
  // Cast is safe since the constructor ensures that the impl object is of the derived type.
  return static_cast<impl::CompositeComponentImplementation const &>(Component::implementation());
}

} // namespace visr
