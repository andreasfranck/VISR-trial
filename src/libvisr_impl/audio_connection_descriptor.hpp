/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRIL_AUDIO_CONNECTION_DESCRIPTORHPP_INCLUDED
#define VISR_LIBRIL_AUDIO_CONNECTION_DESCRIPTORHPP_INCLUDED

#include <ciso646>
#include <cstddef>
#include <initializer_list>
#include <set>
#include <string>
#include <vector>

namespace visr
{
namespace ril
{
class AudioPortBase;
class Component;

class AudioChannelSlice
{
public:
  using IndexType = std::size_t;
  using StrideType = std::ptrdiff_t;

  /**
   * Default constructor, construct an empty range.
   */
  AudioChannelSlice();

  explicit AudioChannelSlice( IndexType val );

  explicit AudioChannelSlice( IndexType start, IndexType size );

  /**
   * Construct a slice from a full start,end,stride specification
   * @throw std::invalid_argument if a resulting index would be negative
   */
  explicit AudioChannelSlice( IndexType start, IndexType size, StrideType stride );

  /**
   * @throw std::invalid_argument if a resulting index would be negative
   */
  void set( IndexType start, IndexType size = 1, StrideType stride = 1 );

  void clear();

  IndexType size() const { return mSize; }

  IndexType start() const { return mStart; }

  StrideType stride() const { return mStride; }

  /**
   * Return the channel index at the \p idx-th position.
   * @throw std::out_of_range if \p idx exceeds the size of the slice
   */
  IndexType at( IndexType idx ) const;

  /**
  * Return the channel index at the \p idx-th position.
  * @note No checking is performed whrther \p idx exceeds the size of the slice.
  */
  IndexType operator[]( IndexType idx ) const;

  /**
   * Convenience function to write the indices generated by the slice into an output iterator
   * Todo: Decide whether this exposes too much
   */
  template<typename OutputIterator>
  OutputIterator writeIndices( OutputIterator outIt ) const
  {
    for( std::size_t idx( 0 ); idx < size(); ++idx )
    {
      *outIt = operator[]( idx );
      ++outIt;
    }
    return outIt;
  }

private:
  IndexType mStart;
  IndexType mSize;
  StrideType mStride;
};

class AudioChannelIndexVector
{
public:
  using IndexType = AudioChannelSlice::IndexType;
  using ContainerType = std::vector<IndexType>;
  using const_iterator = ContainerType::const_iterator;

  AudioChannelIndexVector();

  explicit AudioChannelIndexVector( std::vector<IndexType> const & indices );

  explicit AudioChannelIndexVector( std::initializer_list<IndexType> const & indices );

  explicit AudioChannelIndexVector( AudioChannelSlice const & slice );

  explicit AudioChannelIndexVector( std::initializer_list<AudioChannelSlice> const & slices );


  std::size_t size() const { return mIndices.size(); }

  IndexType operator[]( std::size_t idx ) const
  {
    return mIndices[idx];
  }

  IndexType& operator[]( std::size_t idx )
  {
    return mIndices[idx];
  }


  IndexType at( std::size_t idx ) const;  

  const_iterator begin() const { return mIndices.begin(); }
  const_iterator end( ) const { return mIndices.end(); }

private:

  std::vector<IndexType> mIndices;
};

#if 0
struct AudioPortDescriptor
{
public:
  AudioPortDescriptor()
   : mComponent( nullptr )
   , mPort( nullptr )
  {
  }

  explicit AudioPortDescriptor( Component * pComponent, AudioPort * pPort );

  bool operator<(AudioPortDescriptor const & rhs) const;

  bool valid() const { return mComponent and mPort; }

  Component const & component() const { return *mComponent; }
  AudioPort const & port() const { return *mPort; }

private:
  Component* mComponent;
  AudioPort* mPort;
};
#endif

/**
 * Store data from definition in derived class until initialisation of runtime structures.
 * @todo This should move into CompositeComponent.
 */
struct AudioConnection
{
public:
  /**
   * Default constructor, required for use in standard containers.
   * Creates a struct with empty strings for all members.
   */
  AudioConnection()
   : mSender( nullptr )
   , mReceiver( nullptr )
  {
  }

  AudioConnection( AudioPortBase * pSender,
                   AudioChannelIndexVector const & pSendIndices,
                   AudioPortBase * pReceiver,
                   AudioChannelIndexVector const & pReceiveIndices );
#if 0
  AudioConnection( std::string const & pSendComponent,
                   std::string const & pSendPort,
                   AudioChannelIndexVector const & pSendIndices,
                   std::string const & pReceiveComponent,
                   std::string const & pReceivePort,
                   AudioChannelIndexVector const & pReceiveIndices );
#endif
  bool operator<(AudioConnection const & rhs) const;

  AudioPortBase * sender() const { return mSender; }
  AudioPortBase * receiver() const { return mReceiver; }

  AudioChannelIndexVector const & sendIndices() const { return mSendIndices; }
  AudioChannelIndexVector const & receiveIndices( ) const { return mReceiveIndices; }

private:
  AudioPortBase * mSender;
  AudioPortBase * mReceiver;
  AudioChannelIndexVector const mSendIndices;
  AudioChannelIndexVector const mReceiveIndices;
};

using AudioConnectionTable = std::multiset< AudioConnection >;

} // namespace ril
} // namespace visr

#endif // #ifndef VISR_LIBRIL_AUDIO_CONNECTION_DESCRIPTORHPP_INCLUDED
