/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <libril/composite_component.hpp>

#include "composite_component_implementation.hpp"

namespace visr
{

CompositeComponent::CompositeComponent( SignalFlowContext& context,
                                        char const * name,
                                         CompositeComponent * parent /*= nullptr*/ )
 : Component( context, name, parent )
 , mImpl( new impl::CompositeComponent( *this) )
{
}

CompositeComponent::~CompositeComponent()
{
}

bool CompositeComponent::isComposite() const
{
  return true;
}

std::size_t CompositeComponent::numberOfComponents() const
{
  return mImpl->numberOfComponents();
}

void CompositeComponent::registerParameterConnection( std::string const & sendComponent,
                                                      std::string const & sendPort,
                                                      std::string const & receiveComponent,
                                                      std::string const & receivePort )
{
  mImpl->registerParameterConnection( sendComponent, sendPort, receiveComponent, receivePort );
}

void CompositeComponent::registerParameterConnection( ParameterPortBase & sender,
                                                      ParameterPortBase & receiver )
{
  mImpl->registerParameterConnection( sender, receiver );
}

void CompositeComponent::registerAudioConnection( std::string const & sendComponent,
                                                  std::string const & sendPort,
                                                  ChannelList const & sendIndices,
                                                  std::string const & receiveComponent,
                                                  std::string const & receivePort,
                                                  ChannelList const & receiveIndices )
{
  mImpl->registerAudioConnection( sendComponent, sendPort, sendIndices,
                                  receiveComponent, receivePort, receiveIndices );
}

void CompositeComponent::registerAudioConnection( AudioPortBase & sendPort,
                              ChannelList const & sendIndices,
                              AudioPortBase & receivePort,
                              ChannelList const & receiveIndices )
{
  mImpl->registerAudioConnection( sendPort, sendIndices, receivePort, receiveIndices );
}


void CompositeComponent::registerAudioConnection( AudioPortBase & sendPort,
                                                  AudioPortBase & receivePort )
{
  mImpl->registerAudioConnection( sendPort, receivePort );
}

impl::CompositeComponent & CompositeComponent::implementation()
{
  return *mImpl;
}

impl::CompositeComponent const & CompositeComponent::implementation() const
{
  return *mImpl;
}

} // namespace visr
