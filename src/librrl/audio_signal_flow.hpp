/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_LIBRRL_AUDIO_SIGNAL_FLOW_HPP_INCLUDED
#define VISR_LIBRRL_AUDIO_SIGNAL_FLOW_HPP_INCLUDED

#include "audio_interface.hpp"

// TODO: Replace by forward declarations if possible
#include "audio_connection_map.hpp"
#include "parameter_connection_map.hpp"

#include <libril/audio_port_base.hpp>
#include <libril/communication_protocol_base.hpp>
#include <libril/constants.hpp>
#include <libril/signal_flow_context.hpp>

#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

namespace visr
{
// Forward declarations
class AtomicComponent;
class ParameterPortBase;

namespace impl
{
class ComponentImplementation;
class CompositeComponentImplementation;
}

namespace rrl
{

// Forward declarations
class AudioSignalPool;

/**
 * Base class for signal flows, i.e., graphs of connected audio
 * components which perform an audio signal processing operation.
 * This base class provides the infrastructure for setting up the
 * graphs and for transferring the input and output samples.
 * For the audio processing, this class provides a callback interface
 * that must be called in regular intervals (i.e., for a fixed number
 * of samples consumed and generated, respectively.
 * This class is abstract, i.e., cannot be instantiated, but must be subclassed.
 */
class AudioSignalFlow
{
public:
  using SignalIndexType = std::size_t; // TODO: Check whether to introduce a consistently used type alias for indices

  /**
   * Constructor.
   * @param period The number of samples processed in each invocation
   * of the process() function.
   * @param samplingFrequency The sampling frequency associated with
   * the discrete-time signals to be processed.
   */
  explicit AudioSignalFlow( Component & flow );

  /**
   * Destructor.
   */
  ~AudioSignalFlow();

  /**
   * Method to transfer the capture and playback samples to and from
   * the locations where they are expected, and execute the contained atomic components.
   * Called from processFunction(). For a parameter description
   * (except userData), see @see processFunction().
   */
  AudioInterface::CallbackResult
  process( SampleType const * const * captureSamples,
                        SampleType * const * playbackSamples );

  /**
   * A static, i.e., non-class function which can be registered as a
   * callback method. Calling this method triggers the transfer of the
   * passed samples and the invocation of the process() function of
   * the derived subclasses.
   * The method must only only be called after the initialisation of the
   * class is complete.
   * @param userData An opaque pointer, must hold the 'this' pointer
   * of the AudioSignalFlow object
   * @param captureSamples A pointer array to arrays of input samples
   * to be processed. The pointer array must hold numberOfCaptureChannels()
   * elements, and each sample array must hold period() samples.
   * @param playbackSamples  A pointer array to arrays of output samples
   * to hold the results of the operation. The pointer array must hold numberOfPlaybackChannels()
   * elements, and each sample array must hold period() samples.
   * @param callbackResult A enumeration type to hold the result of
   * the process() function. Typically used to signal error conditions
   * or to request termination.
   * @TODO After the redesign, the translation to a callback function (and discarding the object pointer) needs to be done somewhere else!
   */
  static void  processFunction( void* userData,
                                SampleType const * const * captureSamples,
                                SampleType * const * playbackSamples,
                                AudioInterface::CallbackResult& callbackResult );

  /**
   * Query methods.
   */
  //@{
  /**
   * Query whether the signal flow has been set up successfully.
   * The corresponding data member must be set in the constructor or a specific initialisation method.
   * @todo After removal of the setup method and performing the setup in the constructor, consider removal of this mechanism.
   */
  bool initialised() const { return mInitialised; }

  /**
   * Return the number of samples processed in each process() function
   * @note At the moment this is required by the Python binding.
   */
  std::size_t period() const;

  std::size_t numberOfAudioCapturePorts( ) const;

  std::size_t numberOfAudioPlaybackPorts( ) const;

  /**
  * Return the name of the capture port indexed by \p idx
  * @throw std::out_of_range If the \p idx exceeds the number of capture ports.
  */
  char const * audioCapturePortName( std::size_t idx ) const;

  /**
   * Return the name of the playback port indexed by \p idx
   * @throw std::out_of_range If the \p idx exceeds the number of playback ports.
   */
  char const * audioPlaybackPortName( std::size_t idx ) const;

  /**
   * Query the width of the capture port, i.e., the number of external
   * inputs of the graph.
   * @throw std::logic_error if the method is called before the object
   * is initialised.
   */
  std::size_t numberOfCaptureChannels() const;
  
  /**
   * Query the width of the playback port, i.e., the number of external
   * outputs of the graph.
   * @throw std::logic_error if the method is called before the object
   * is initialised.
   */
  std::size_t numberOfPlaybackChannels() const;
  //@}

  /**
   * Query and access external parameter ports.
   */
  //@{

  using ProtocolReceiveEndpoints = std::map<std::string, std::unique_ptr<CommunicationProtocolBase::Output> >;
  using ProtocolSendEndpoints = std::map<std::string, std::unique_ptr<CommunicationProtocolBase::Input> >;

  std::size_t numberExternalParameterReceivePorts() const
  {
    return mProtocolReceiveEndpoints.size();
  }

  std::size_t numberExternalParameterSendPorts() const
  {
    return mProtocolSendEndpoints.size();
  }

  ProtocolReceiveEndpoints const & externalParameterReceiveEndpoints() const
  {
    return mProtocolReceiveEndpoints;
  }

  ProtocolSendEndpoints const & externalParameterSendEndpoints() const
  {
    return mProtocolSendEndpoints;
  }

  /**
   * Return a input protocol for a named top-level parameter port.
   * @throw std::out_of_range No top-level parameter port with this name exists.
   */
  CommunicationProtocolBase::Output & externalParameterReceivePort( char const * portName );

  /**
  * Return a output protocol for a named top-level parameter port.
  * @throw std::out_of_range No top-level parameter port with this name exists.
  */
  CommunicationProtocolBase::Input & externalParameterSendPort( char const * portName );

  //@}


private:
  bool initialiseAudioConnections( std::ostream & messages, AudioConnectionMap const & originalConnections, AudioConnectionMap & finalConnections);

  /**
    * Initialise the parameter infrastructure.
    * @return True if the initialisation was successful, false otherwise. In this case, \p messages should provide an explanation.
    * @param [out] messages Output stream containing error messages and warnings generated during the initialisation.
    * @param [out] parameterConnections Connection map to be filled during the initialisation process.
    */
  bool initialiseParameterInfrastructure( std::ostream & messages, ParameterConnectionMap const & originalConnections, ParameterConnectionMap & finalConnections );

  /**
   * Initialise the schedule for executing the contained elements.
   * @return Boolean value indicating whether the initialisation was successful.
   * @param [out] messages Output stream containing error messages and warnings generated during the initialisation.
   * @param audioConnections The audio connection relations of the final signal flow (possibly including additional infrastructure components created during initialisation)
   * @param parameterConnections The parameter connection relations of the final signal flow (possibly including additional infrastructure components created during initialisation)
   */
  bool initialiseSchedule( std::ostream & messages,
                           AudioConnectionMap const & audioConnections,
                           ParameterConnectionMap const & parameterConnections );

  /**
   * Mark the signal flow as "initialised".
   * @todo Decide whether this is the right place for a consistency check.
   */
  void setInitialised( bool newState = true ) { mInitialised = newState; }

  std::size_t numberCommunicationProtocols() const;

  /**
  * Method called within the processFunction callback to execute the atomic components of the graph
  */
  void executeComponents( );

  /**
   * The signal flow handled by this object.
   * Can be either an atomic or a (hierarchical) composite component/
   */
  impl::ComponentImplementation & mFlow;

  /**
   * Flag stating whether the signal flow is fully initialised.
   * @note: As long as initialisation is performed fully in the constructor, here is no need for that,
   */
  bool mInitialised;

  /**
   * Parameter infrastructure
   */
  //@{

  using CommunicationProtocolContainer = std::vector<std::unique_ptr<CommunicationProtocolBase> >;

  CommunicationProtocolContainer mCommunicationProtocols;
  //@}

  /**
   * Audio connection infrastructure.
   */
  //@{

  std::vector<impl::AudioPortBaseImplementation *> mToplevelInputs;

  std::vector<impl::AudioPortBaseImplementation *> mToplevelOutputs;
  //@}

  /**
  * The communication area for this signal flow.
  */
  std::unique_ptr<AudioSignalPool> mAudioSignalPool;

  /**
   * These ports are the top-level system inputs and outputs.
   * They correspond to the capture and playback indices.
   * @note at the moment the order of the ports is determined by the system.
   */
  //@{
  std::vector < impl::AudioPortBaseImplementation*> mTopLevelAudioInputs;
  std::vector < impl::AudioPortBaseImplementation*> mTopLevelAudioOutputs;
  //@}

  std::vector<char*> mCaptureChannels;
  std::vector<char*> mPlaybackChannels;

  /**
   * Data structures to hold top-level parameter ports (or their input/output facilities)
   */
  //@{
  ProtocolReceiveEndpoints mProtocolReceiveEndpoints;

  ProtocolSendEndpoints mProtocolSendEndpoints;

  //@}

  using InternalComponentList = std::vector<std::unique_ptr<AtomicComponent> >;
    
  InternalComponentList mInfrastructureComponents;

  using ProcessingSchedule = std::vector<AtomicComponent * >;

  ProcessingSchedule mProcessingSchedule;
};

} // namespace rrl
} // namespace visr

#endif // #ifndef VISR_LIBRRL_AUDIO_SIGNAL_FLOW_HPP_INCLUDED
