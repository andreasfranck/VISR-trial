/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_PML_SIGNAL_ROUTING_PARAMETER_HPP_INCLUDED
#define VISR_PML_SIGNAL_ROUTING_PARAMETER_HPP_INCLUDED

#include "empty_parameter_config.hpp"

#include <libril/parameter_type.hpp>
#include <libril/typed_parameter_base.hpp>

#include <algorithm>
#include <ciso646>
#include <cstdint>
#include <climits>
#include <initializer_list>
#include <stdexcept>
#include <set>
#include <tuple>

namespace visr
{
namespace pml
{

/**
 * @note Not sure whether we should introduce parameters to limit 
 */
class SignalRoutingParameter: public ril::TypedParameterBase < pml::EmptyParameterConfig, ril::ParameterType::SignalRouting >
{
public:
  using IndexType = std::size_t;

  /**
   * @note: std::numeric_limits<IndexType> would be nicer, but MSVC does not support constexpr yet.
   */ 
  static const IndexType cInvalidIndex = UINT_MAX;

  /**
   * Structure for a single routing entry
   * @note In order to enable initialisation from a brace-enclosed list, this class is a plain datatype without constructors or private members.
   */
  struct Entry
  {
  public:
    IndexType input;
    IndexType output;
  };

  class CompareEntries
  {
  public:
    bool operator()( Entry const & lhs, Entry const & rhs ) const
    {
      return lhs.output < rhs.output;
    }
  };

  /**
   * Data type used for representing routings.
   */
  using RoutingsType = std::set< Entry, CompareEntries >;


  /**
   * Default constructor, creates an empty list
   */
  SignalRoutingParameter() {}

  SignalRoutingParameter( std::initializer_list<Entry> const & entries );

  SignalRoutingParameter( const SignalRoutingParameter & rhs ) = default;

  void swap( SignalRoutingParameter& rhs );

  bool empty() const { return mRoutings.empty(); }

  std::size_t size() const { return mRoutings.size(); }

  SignalRoutingParameter & operator=(SignalRoutingParameter const & rhs) = default;

  RoutingsType::const_iterator begin() const { return mRoutings.begin(); }

  RoutingsType::const_iterator end() const { return mRoutings.end(); }

  void addRouting( IndexType inputIdx, IndexType outputIdx )
  {
    addRouting( Entry{ inputIdx, outputIdx } );
  }

  void addRouting( Entry const & newEntry );

  bool removeEntry( Entry const & entry );

  bool removeEntry( IndexType outputIdx );

  Entry const & getEntry( IndexType outputIdx ) const
  {
    static const Entry returnInvalid{ cInvalidIndex, cInvalidIndex };

    RoutingsType::const_iterator const findIt = mRoutings.find( Entry{ cInvalidIndex, outputIdx } );
    return findIt == mRoutings.end() ? returnInvalid : *findIt;
  }

  IndexType getInput( IndexType outputIdx ) const
  {
    RoutingsType::const_iterator const findIt = mRoutings.find( Entry{ cInvalidIndex, outputIdx } );
    return findIt == mRoutings.end() ? cInvalidIndex : findIt->input;
  }

  IndexType getOutput( IndexType inputIdx ) const;

  bool parse( std::string const & encoded );

private:
  RoutingsType mRoutings;
};

} // namespace pml
} // namespace visr

DEFINE_PARAMETER_TYPE( visr::pml::SignalRoutingParameter, visr::ril::ParameterType::SignalRouting, visr::pml::EmptyParameterConfig )

#endif // VISR_PML_SIGNAL_ROUTING_PARAMETER_HPP_INCLUDED
