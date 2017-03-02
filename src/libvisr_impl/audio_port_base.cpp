/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include <libril/audio_port_base.hpp>

#include <libril/component.hpp>

#include <libvisr_impl/audio_port_base_implementation.hpp>

namespace visr
{

AudioPortBase::AudioPortBase( std::string const & name, Component & container, AudioSampleType::Id sampleType, PortBase::Direction direction )
 : AudioPortBase( name, container, sampleType, direction, 0 )
{
}

AudioPortBase::AudioPortBase( std::string const & name, Component& container, AudioSampleType::Id sampleType, PortBase::Direction direction, std::size_t width )
  : mImpl( new impl::AudioPortBaseImplementation( name, *this, &container.implementation(), sampleType, direction, width ) )
{
}

AudioPortBase::~AudioPortBase() = default;

void AudioPortBase::setWidth( std::size_t newWidth )
{
  mImpl->setWidth( newWidth );
}

std::size_t AudioPortBase::width() const noexcept
{
  return mImpl->width();
}

std::size_t AudioPortBase::channelStrideSamples() const noexcept
{
  return mImpl->channelStrideSamples();
}

void * AudioPortBase::basePointer()
{
  return mImpl->basePointer();
}

void const * AudioPortBase::basePointer() const
{
  return mImpl->basePointer();
}

impl::AudioPortBaseImplementation & AudioPortBase::implementation()
{
  return *mImpl;
}

impl::AudioPortBaseImplementation const & AudioPortBase::implementation() const
{
  return *mImpl;
}


} // namespace visr
