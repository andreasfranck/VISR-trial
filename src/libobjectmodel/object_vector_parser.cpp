/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "object_vector_parser.hpp"

#include "object_factory.hpp"
#include "object_parser.hpp"
#include "object_type.hpp"
#include "object_vector.hpp"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <ciso646>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace visr
{
namespace objectmodel
{

/*static*/ void ObjectVectorParser::
fillObjectVector( std::string const & message, ObjectVector & res )
{
  std::istringstream streamObj( message );
  fillObjectVector( streamObj, res );
}

/*static*/ void ObjectVectorParser::
fillObjectVector( std::basic_istream<char> & message, ObjectVector & res )
{
  // Provide strong exception safety by adding the new elements to a new object 
  // vector and swapping the vectors at the end of the function.
  ObjectVector newVec;

  updateObjectVector( message, newVec );

  newVec.swap( res );
}

/*static*/ void ObjectVectorParser::
fillObjectVector( char const * message, ObjectVector & res )
{
  fillObjectVector( std::string(message), res );
}

/*static*/ void ObjectVectorParser::
updateObjectVector( std::string const & message, ObjectVector & res )
{
  std::istringstream streamObj( message );
  updateObjectVector( streamObj, res );
}

/*static*/ void ObjectVectorParser::
updateObjectVector( std::basic_istream<char> & message, ObjectVector & res )
{
  using ptree = boost::property_tree::ptree;

  ptree propTree;
  try
  {
    // TODO: Should we restrict ourselved to JSON at this level (or should we move the decision up one level?)
    read_json( message, propTree );
  }
  catch( std::exception const & ex )
  {
    throw std::invalid_argument( std::string( "Error while parsing a json object message: " ) + ex.what() );
  }
//  BOOST_FOREACH( ptree::value_type &v, propTree.get_child( "objects") )
  for( auto v: propTree.get_child( "objects") )
  {
    try
    {
      parseObject( v.second, res );
    }
    catch( std::exception const & ex )
    {
      throw std::invalid_argument( std::string( "Error while parsing the json message content: " ) + ex.what( ) );
    }
  }
}

/*static*/ void ObjectVectorParser::
updateObjectVector( char const * message, ObjectVector & res )
{
  updateObjectVector( std::string( message ), res );
}

/*static*/void ObjectVectorParser::
parseObject( boost::property_tree::ptree const & subtree, ObjectVector & res )
{
  // properties common to all source types
  std::string const objTypeStr = subtree.get<std::string>( "type" );
  ObjectId const objId = subtree.get<ObjectId>( "id" );

  // might throw (if the string does not match a recognised object type
  ObjectTypeId const objTypeId = stringToObjectType( objTypeStr );

  // Create a parser for this type (used later)
  ObjectParser const & objParser = ObjectFactory::parser( objTypeId );

  // This contains some redundancy with the ObjectParser::parse() call called later,
  // but this probably unavoidable.
  ObjectId newId = subtree.get<ObjectId>( "id" );

  ObjectVector::iterator findIt = res.find( newId );
  bool const found = findIt != res.end();

  // This implementation requires the construction of a new object even if a matching object is found, but provides exception
  // safety, i.e., resets the original state of the object to be by modified, which is thus unchanged if the parser throws
  std::unique_ptr< Object > newObj( (found and (findIt->type() == objTypeId))
                                   ? findIt->clone()
                                   : ObjectFactory::create(objTypeId, objId ) );

  objParser.parse( subtree, *newObj );

  res.insert( std::move(newObj) );
}

/*static*/ void ObjectVectorParser::encodeObjectVector( ObjectVector const & objects,
                                                        std::basic_ostream<char> & message )
{
  boost::property_tree::ptree vecTree;  // new empty tree;
  boost::property_tree::ptree objListTree;
  for( ObjectVector::const_iterator runIt( objects.cbegin()); runIt != objects.cend(); ++runIt )
  {
    boost::property_tree::ptree objTree;
    Object const & obj( *runIt );

    // Create a parser for this type
    ObjectParser const & objParser = ObjectFactory::parser( obj.type() );
    objParser.write( obj, objTree );
    objListTree.push_back( std::make_pair("",objTree) );
  }
  vecTree.put_child( "objects", objListTree );

  boost::property_tree::write_json( message, vecTree, false /*no pretty priniting */ );
}

} // namespace objectmodel
} // namespace visr
