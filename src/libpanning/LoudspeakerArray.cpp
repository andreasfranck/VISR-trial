//
//  LoudspeakerArray.cpp
//
//  Created by Dylan Menzies on 18/11/2014.
//  Copyright (c) ISVR, University of Southampton. All rights reserved.
//

// avoid annoying warning about unsafe STL functions.
#ifdef _MSC_VER 
#pragma warning(disable: 4996)
#endif

#include "LoudspeakerArray.h"

#include <libefl/degree_radian_conversion.hpp>
#include <libefl/cartesian_spherical_conversion.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <ciso646>
#include <cstdio>
#include <iterator>
#include <set>
#include <tuple>

#define PI 3.1412659f

namespace visr
{
namespace panning
{


LoudspeakerArray::LoudspeakerArray()
 : m_is2D( false )
 , m_isInfinite( false )
{
}

int LoudspeakerArray::load( FILE *file )
{
  int n, i, chan;
  char c;
  Afloat x, y, z;
  Afloat az, el, r;
  int l1, l2, l3;
  int nSpk, nTri;

  i = nSpk = nTri = 0;

  struct SpkStruct
  {
    int id;
    XYZ pos;
    ChannelIndex channel;
  };
  struct CmpSpkStruct
  {
    bool operator()( SpkStruct const & lhs, SpkStruct const & rhs ) const
    {
      return lhs.id < rhs.id;
    }
  };
  std::set<SpkStruct, CmpSpkStruct>  tmpSpeakers;
  std::vector<std::array<LoudspeakerIndexType, 3> > tmpTriplets;

  m_is2D = false;
  m_isInfinite = false;

  if( file == 0 ) return -1;

  do
  {
    c = fgetc( file );

    if( c == 'c' )
    {        // cartesians
      n = fscanf( file, "%d %d %f %f %f\n", &i, &chan, &x, &y, &z );
      if( n != 5 )
      {
        return -1;
      }
#if 0
      if (i <= MAX_NUM_SPEAKERS) {
        setPosition(i,x,y,z,m_isInfinite);
        setChannel(i,chan);
        if (i > nSpk) nSpk = i;
      }
#else
      SpkStruct newSpk;
      newSpk.id = i;
      newSpk.pos.set( x, y, z, m_isInfinite );
      newSpk.channel = chan;
      // Insert the new speaker description 
      bool insRet;
      std::tie( std::ignore, insRet ) = tmpSpeakers.insert( newSpk );
      if( !insRet )
      {
        return -1; // Insertion failed, i.e., duplicated index
      }
#endif
    }
    else if( c == 'p' )
    {   // polars, using degrees
      n = fscanf( file, "%d %d %f %f %f\n", &i, &chan, &az, &el, &r );
      if( n != 5 )
      {
        return -1;
      }
#if 0
      if( i <= MAX_NUM_SPEAKERS ) {
        az *= PI/180;
        el *= PI/180;
        xy = r*cos(el);
        x = xy*cos(az);
        y = xy*sin(az);
        z = r*sin(el);
        setPosition(i,x,y,z,m_isInfinite);
        setChannel(i,chan);
        if (i > nSpk) nSpk = i;
      }
#else
      std::tie( x, y, z ) = efl::spherical2cartesian( efl::degree2radian( az ),
                                                      efl::degree2radian( el ),
                                                      r );
      SpkStruct newSpk;
      newSpk.id = i;
      newSpk.pos.set( x, y, z, m_isInfinite );
      newSpk.channel = chan;
      // Insert the new speaker description 
      bool insRet;
      std::tie( std::ignore, insRet ) = tmpSpeakers.insert( newSpk );
      if( !insRet )
      {
        return -1; // Insertion failed, i.e., duplicated index
      }
#endif
    }
    else if( c == 't' )
    {    // tuplet - triplet or duplet
      n = fscanf( file, "%d %d %d %d\n", &i, &l1, &l2, &l3 );
      if( n < 3 || n > 4 )
      {
        return -1;
      }
#if 0
      if( i <= MAX_NUM_LOUDSPEAKER_TRIPLETS ) {
        if (n == 3) l3 = 1;
        setTriplet(i-1, l1-1, l2-1, l3-1);
        if (i > nTri) nTri = i;
      }
#else
      // The triplet index does not matter
      std::array<LoudspeakerIndexType, 3> triplet = { l1, l2, l3 };
      tmpTriplets.push_back( triplet );
#endif
    }
    else if( c == '2' )
    {    // switch to '2D' mode
      m_is2D = true;
    }
    else if( c == 'i' )
    {    // switch to 'infinite' mode
      m_isInfinite = true;
    }
    else if( c == '%' )
    {    // comment
      while( fgetc( file ) != '\n' && !feof( file ) );
    }

  }
  while( !feof( file ) );

  std::size_t numSpeakers = tmpSpeakers.size();
  m_position.resize( numSpeakers );
  m_channel.resize( numSpeakers );
  std::size_t spkIdx = 0; 
  for( auto const & v : tmpSpeakers )
  {
    // Check that the (ordered) speakers are consecutively ordered from 1.
    if( v.id != spkIdx+1 ) // Speakers are still one-indexed.
    {
      return -1;
    }
    m_channel[spkIdx] = v.channel;
    m_position[spkIdx] = v.pos;
    ++spkIdx;
  }

  std::size_t const numTriplets = tmpTriplets.size();
  m_triplet.resize( numTriplets );
  for( std::size_t tripletIdx( 0 ); tripletIdx < numTriplets; ++tripletIdx )
  {
    std::array<LoudspeakerIndexType, 3> const & src = tmpTriplets[tripletIdx];
    TripletType & dest = getTriplet( tripletIdx );
    dest[0] = src[0] - 1; // the collected indices are one-offset, so we need to convert them.
    dest[1] = src[1] - 1;
    dest[2] = src[2] - 1;
  }
  return 0;
}

namespace // unnamed
{

/**
 * Local function to parse the coordinate of either a normal or a virtual loudspeaker.
 * @param node The speaker node
 * @param isInfinite The global infinity flag for the array, i.e., whether loudspeakers are considered to emit plane waves.
 * @return Reference to return the parsed position.
 * @throw std::invalid_argument If parsing fails.
 */
XYZ parseCoordNode( boost::property_tree::ptree const & node, bool isInfinite )
{
  XYZ pos(0.0f, 0.0f, 0.0f, isInfinite );
  using namespace boost::property_tree;
  std::size_t numCartCoords = node.count( "cart" );
  std::size_t numPolarCoords = node.count( "polar" );

  if( numCartCoords > 1 or numPolarCoords > 1 or not( (numCartCoords == 1) xor( numPolarCoords == 1 ) ) )
  {
    throw std::invalid_argument( "LoudspeakerArray::loadXml(): For each speaker, exactly one \"cart\" or \"polar\" node must exist.`" );
  }
  if( numCartCoords == 1 )
  {
    ptree::const_assoc_iterator cartIt = node.find( "cart" );
    assert( cartIt != node.not_found( ) );
    ptree const coordNode = cartIt->second;
    pos.x = coordNode.get<Afloat>( "<xmlattr>.x" );
    pos.y = coordNode.get<Afloat>( "<xmlattr>.y" );
    pos.z = coordNode.get<Afloat>( "<xmlattr>.z" );
  }
  else
  {
    ptree::const_assoc_iterator polarIt = node.find( "polar" );
    assert( polarIt != node.not_found( ) );
    ptree const coordNode = polarIt->second;
    Afloat const az = coordNode.get<Afloat>( "<xmlattr>.az" );
    Afloat const el = coordNode.get<Afloat>( "<xmlattr>.el" );
    Afloat const r = coordNode.get<Afloat>( "<xmlattr>.r" );
    std::tie( pos.x, pos.y, pos.z ) = efl::spherical2cartesian( efl::degree2radian( az ), efl::degree2radian( el ), r );
  }
  return pos;
}

} // unnamed namespace


void LoudspeakerArray::loadXml( std::string const & filePath )
{
  using namespace boost::property_tree;

  boost::filesystem::path path( filePath );
  if( not exists( path ) or is_directory( path ) )
  {
    throw std::invalid_argument( "ArrayConfiguration::loadXml(): File does not exist." );
  }
  std::ifstream fileStream( path.string( ) );
  if( not fileStream )
  {
    throw std::invalid_argument( "ArrayConfiguration::loadXml(): Invalid file path." );
  }
  boost::property_tree::ptree parseTree;
  boost::property_tree::read_xml( fileStream, parseTree );

  boost::property_tree::ptree treeRoot = parseTree.get_child( "panningConfiguration" );

  boost::optional<std::string> const dimension = treeRoot.get_optional<std::string>( "<xmlattr>.dimension" );
  if( dimension )
  {
    if( dimension.value() == "2" )
    {
      m_is2D = true;
    }
    else if( dimension.value() == "3" )
    {
      m_is2D = false;
    }
    else
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): If given, the \"dimension\" atttribute must be either \"2\" or \"3\"." );
    }
  }
  else
  {
    m_is2D = false;
  }
  boost::optional<bool> const infinity = treeRoot.get_optional<bool>( "<xmlattr>.infinite" );
  m_isInfinite = infinity ? infinity.value() : false;

  const auto speakerNodes = treeRoot.equal_range( "loudspeaker" );
  const auto virtualSpeakerNodes = treeRoot.equal_range( "virtualspeaker" );

  std::size_t const numNormalSpeakers = std::distance( speakerNodes.first, speakerNodes.second );
  std::size_t const numVirtualSpeakers = std::distance( virtualSpeakerNodes.first, virtualSpeakerNodes.second );
  std::size_t const numTotalSpeakers = numNormalSpeakers + numVirtualSpeakers;

  m_position.resize( numTotalSpeakers );
  m_channel.resize( numTotalSpeakers );
  const ChannelIndex cInvalidChannel = std::numeric_limits<ChannelIndex>::max();
  std::fill( m_channel.begin(), m_channel.end(), cInvalidChannel ); // assign special value to check afterwards if every speaker index has been assigned.

  for( ptree::const_assoc_iterator treeIt( speakerNodes.first ); treeIt != speakerNodes.second; ++treeIt )
  {
    ptree const childTree = treeIt->second;
    int id = childTree.get<int>( "<xmlattr>.id" );
    if( id < 1 or id > numTotalSpeakers )
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): The loudspeaker id exceeds the numbeer of loudspeakers." );
    }
    int idZeroOffset = id - 1;
    if( m_channel[idZeroOffset] != cInvalidChannel )
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): Each speaker id must be used exactly once." );
    }
    ChannelIndex const chIdx = childTree.get<ChannelIndex>( "<xmlattr>.channel");
    if( chIdx < 1 )
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): The channel id must be greater or equal than one." );
    }
    m_channel[idZeroOffset] = chIdx - 1;
    m_position[idZeroOffset] = parseCoordNode( childTree, m_isInfinite );
  }
  // Same for the virtual speaker nodes, except there is no 'channel' field.
  for( ptree::const_assoc_iterator treeIt( virtualSpeakerNodes.first ); treeIt != virtualSpeakerNodes.second; ++treeIt )
  {
    ptree const childTree = treeIt->second;
    int id = childTree.get<int>( "<xmlattr>.id" );
    if( id < 1 or id > numTotalSpeakers )
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): The loudspeaker id exceeds the number of loudspeakers." );
    }
    int idZeroOffset = id - 1;
    if( m_channel[idZeroOffset] != cInvalidChannel )
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): Each speaker id must be used exactly once." );
    }
    m_channel[idZeroOffset] = - 1; // set channel id to the reserved value for virtual loudspeakers.
    m_position[idZeroOffset] = parseCoordNode( childTree, m_isInfinite );
  }
  // The checks above (all speaker indices are between 1 and numTotalSpeakers && the indices are unique) are 
  // sufficient to ensure that the spkeaker indices are consecutive.

  // parse the triplet config
  auto const  tripletNodes = treeRoot.equal_range( "triplet" );
  std::size_t const numTriplets = std::distance( tripletNodes.first, tripletNodes.second );
  m_triplet.reserve( numTriplets );
  for( ptree::const_assoc_iterator tripletIt( tripletNodes.first ); tripletIt != tripletNodes.second; ++tripletIt )
  {
    ptree const childTree = tripletIt->second;
    std::array<LoudspeakerIndexType, 3> triplet;
    triplet[0] = childTree.get<LoudspeakerIndexType>( "<xmlattr>.l1" ) - 1;
    triplet[1] = childTree.get<LoudspeakerIndexType>( "<xmlattr>.l2" ) - 1;
    if( m_is2D )
    {
      triplet[2] = -1; // special value reserved for 'unused'
    }
    else
    {
      triplet[2] = childTree.get<LoudspeakerIndexType>( "<xmlattr>.l3" ) - 1;
    }
    if( (triplet[0] < 0) or (triplet[0] >= numTotalSpeakers) 
      or ( triplet[1] < 0 ) or( triplet[1] >= numTotalSpeakers )
      or ( (not m_is2D) and ( (triplet[2] < 0 ) or( triplet[2] >= numTotalSpeakers )) ) )
    {
      throw std::invalid_argument( "LoudspeakerArray::loadXml(): Triplet references non-existing speaker index." );
    }
    m_triplet.push_back( triplet );
  }
  assert( m_triplet.size() == numTriplets );
}

} // namespace panning
} // namespace visr

