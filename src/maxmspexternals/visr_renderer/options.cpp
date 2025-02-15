/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "options.hpp"

#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>

namespace visr
{
namespace maxmsp
{
namespace visr_renderer
{

Options::Options()
 : apputilities::Options()
{
  registerPositionalOption<std::string>( "array-config,c", 1, "Loudspeaker array configuration file" );
  registerOption<std::size_t>( "input-channels,i", "Number of input channels for audio object signal" );
  registerOption<std::size_t>( "output-channels,o", "Number of audio output channels" );
  registerOption<std::size_t>( "object-eq-sections,e", "Number of eq (biquad) section processed for each object signal.");

  registerOption<std::string>( "tracking", "Enable adaptation of the panning using visual tracking. Accepts the position of the tracker in JSON format"
    "\"{ \"port\": <UDP port number>, \"position\": {\"x\": <x in m>, \"y\": <y im m>, \"z\": <z in m> }, \"rotation\": { \"rotX\": rX, \"rotY\": rY, \"rotZ\": rZ } }\" ." );

  registerOption<std::size_t>( "scene-port,r", "UDP port for receiving object metadata" );
}

Options::~Options()
{
}

} // namespace visr_renderer
} // namespace maxmsp
} // namespace visr
