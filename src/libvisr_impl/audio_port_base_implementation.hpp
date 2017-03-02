/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_IMPL_AUDIO_PORT_BASE_IMPLEMENTATION_HPP_INCLUDED
#define VISR_IMPL_AUDIO_PORT_BASE_IMPLEMENTATION_HPP_INCLUDED

#include "port_base_implementation.hpp"

#include <libril/audio_sample_type.hpp>

//#include <cstddef>
//#include <exception>
//#include <iterator>
//#include <limits>
#include <string>
//#include <vector>

#include <valarray>

namespace visr
{
class AudioPortBase;

namespace impl
{

class AudioPortBaseImplementation: public PortBaseImplementation
{
public:

  explicit AudioPortBaseImplementation( std::string const & name, AudioPortBase& containingPort, ComponentImplementation * container, AudioSampleType::Id sampleType, visr::PortBase::Direction direction );

  explicit AudioPortBaseImplementation( std::string const & name, AudioPortBase& containingPort, ComponentImplementation* container, AudioSampleType::Id sampleType, visr::PortBase::Direction direction, std::size_t width );

  virtual ~AudioPortBaseImplementation();

  AudioPortBase & containingPort();

  AudioPortBase const & containingPort() const;

  /**
   * Query whether the port is initialised, e.g., whether a base address and a channel stride has been assigned.
   */
  bool initialised() const noexcept;

  /**
   * Reset the port into uninitialized state, that is, reset the base pointer and channel stride to their initial settings
   */
  void reset() noexcept;

  /**
   * Set the width, i.e., number of single audio channels, for this port.
   * @throw std::logic_error if called when in initialised state.
   */
  void setWidth( std::size_t newWidth );

  /**
   * Return the number of single audio channels allocated for this port
   */
  std::size_t width() const noexcept;

  /**
   * Return the number of samples between the vectors of successive audio channels.
   * If the port has not been initialised, 0 is returned.
   */
  std::size_t channelStrideSamples() const noexcept;

//  std::size_t channelStrideBytes() const noexcept;

  //@}

  void setChannelStrideSamples(std::size_t stride);

//  void setChannelStrideBytes( std::size_t stride );

  void setBasePointer( void* base );

  void setBufferConfig( void* base, std::size_t channelStrideSamples );

  void const * basePointer() const;

  void * basePointer();
protected:
  AudioPortBase & mContainingPort;

  AudioSampleType::Id const cSampleType;

// std::size_t const cSampleSize;

  std::size_t mWidth;

  void * mBasePointer;

  std::size_t mChannelStrideSamples;
};

/**
 * Implementation base class for audio input ports
 * @todo: Move to a separate file if the class is eventually used.
 */
//class AudioInputBase: public AudioPortBaseImplementation
//{
//public:
//  explicit AudioInputBase( std::string const & name, Component * container, AudioSampleType::Id sampleType );
//
//  explicit AudioInputBase( std::string const & name, Component* container, AudioSampleType::Id sampleType, std::size_t width );
//
//  ~AudioInputBase() override;
//
//  void const * basePointer() const;
//};
//
//class AudioOutputBase: public AudioPortBaseImplementation
//{
//public:
//  explicit AudioOutputBase( std::string const & name, Component * container, AudioSampleType::Id sampleType );
//
//  explicit AudioOutputBase( std::string const & name, Component* container, AudioSampleType::Id sampleType, std::size_t width );
//
//  ~AudioOutputBase() override;
//
//  void * basePointer();
//};


} // namespace impl
} // namespace visr

#endif // #ifndef VISR_IMPL_AUDIO_PORT_BASE_IMPLEMENTATION_HPP_INCLUDED
