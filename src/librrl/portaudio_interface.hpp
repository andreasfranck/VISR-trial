/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRRL_PORTAUDIO_INTERFACE_HPP_INCLUDED
#define VISR_LIBRRL_PORTAUDIO_INTERFACE_HPP_INCLUDED

#include <libril/audio_interface.hpp>

#include <libril/communication_area.hpp>
#include <libril/constants.hpp>

#include <portaudio/portaudio.h>

#include <memory>
#include <string>
#include <vector>

namespace visr
{
namespace rrl
{

class PortaudioInterface: public ril::AudioInterface
{
public:
  /**
   * Structure to hold all configuration arguments for a PortAudioInterface instance.
   */
  struct Config
  {
  public:
    /** Default contructor to initialise elements to defined values. */
    Config()
      : mNumberOfCaptureChannels( 0 )
      , mNumberOfPlaybackChannels( 0 )
      , mPeriodSize( 0 )
      , mSampleRate( 0 )
      , mSampleFormat( SampleFormat::float32Bit )
      , mInterleaved( false )
      , mHostApi( "" )
    {}

    std::size_t mNumberOfCaptureChannels;
    std::size_t mNumberOfPlaybackChannels;

    std::size_t mPeriodSize;

    /**
     * Todo: Consider moving this definition to a more general place.
    */
    using SamplingRateType = std::size_t;
    SamplingRateType mSampleRate;

    /**
     * Enumeration for a type-independent sample format specification
     * TODO: Move to a more general location (for use by all audio interfaces)
     */
    class SampleFormat
    {
    public:
      enum Type
      {
        signedInt8Bit,
        unsignedInt8Bit,
        signedInt16Bit,
        unsignedInt16Bit,
        signedInt24Bit,
        unsignedInt24Bit,
        signedInt32Bit,
        unsignedInt32Bit,
        float32Bit
      };
    };
    SampleFormat::Type mSampleFormat;

    bool mInterleaved;

    /**
     * A string determining the host API to be used for portaudio.
     * At the moment, admissible values are 'default' on all platforms 'DirectSound', 'MME', 'ASIO' 'SoundManager', 'CoreAudio', 'OSS', ALSA', AL',
     * 'WDMKS', 'JACK''WASAPI'
     */
    std::string mHostApi;

  };

  using Base = ril::AudioInterface;

  explicit PortaudioInterface( Config const & config );

  ~PortaudioInterface( );

  /* virtual */ void start();

  /* virtual */ void stop();

  /*virtual*/ bool registerCallback( AudioCallback callback, void* userData );

  /*virtual*/ bool unregisterCallback( AudioCallback audioCallback );
private:
  static int sEngineCallback( const void *input,
                              void *output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData );

  int engineCallbackFunction( const void *input,
                              void *output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags );

  /**
   * Convert and transfer the audio samples from the input
   * of the Portaudio callback function to an array of sample buffers
   * where it is passed to the audio processing callback.
   */
  void transferCaptureBuffers( void const * input );

  /**
   * Transfer and convert the audio samples generated by the audio
   * processing callback to the output argument of the Portaudio callback
   * function.
   */
  void transferPlaybackBuffers( void * output );

  std::size_t const mPeriodSize;

  std::size_t const mNumCaptureChannels;

  std::size_t const mNumPlaybackChannels;

  Config::SampleFormat::Type mSampleFormat;

  std::size_t mSampleRate;

  bool const mInterleaved;

  std::string const mHostApiName;

  PaStream * mStream;

  Base::AudioCallback mCallback;

  void* mCallbackUserData;

  std::unique_ptr<ril::CommunicationArea<ril::SampleType> > mCommunicationBuffer;

  /**
   * Buffer to hold the pointers to the sample vectors for the input
   * of the audio processor.
   * These samples are written from the capture ports of the sound
   * interface and then passed to the audio processing callback function.
   */
  std::vector< ril::SampleType * > mCaptureSampleBuffers;

  /**
   * Buffer to hold the pointers to the sample vectors for the output
   * of the audio processor.
   * These samples are generated by the audio processing callback
   * function and then passed to the playback argument of the
   * portaudio callback function.
   */
  std::vector< ril::SampleType * > mPlaybackSampleBuffers;

};

} // namespace rrl
} // namespace visr

#endif // #ifndef VISR_LIBRRL_PORTAUDIO_INTERFACE_HPP_INCLUDED
