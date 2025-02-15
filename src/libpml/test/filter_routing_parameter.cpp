/* Copyright Institue of Sound and Vibration Research - All rights reserved. */

#include <libpml/filter_routing_parameter.hpp>

#include <boost/test/unit_test.hpp>

#include <ciso646>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace visr
{
namespace pml
{
namespace test
{

BOOST_AUTO_TEST_CASE( FilterRoutingParameterInstantiation )
{
  FilterRoutingParameter const defaultObj;

  FilterRoutingParameter const obj1( 0, 1, 2 );

  // Construct from initialiser 
  FilterRoutingParameter const obj1a{ 0, 1, 2 };

  FilterRoutingParameter const obj2( 1, 0, 0, 0.3 );
  
  FilterRoutingParameter const copyObj( obj1 );

  BOOST_CHECK( obj1.filterIndex == copyObj.filterIndex );

  FilterRoutingListParameter emptyList;

  FilterRoutingListParameter initList( { { 0, 0, 2, 0.7 }, { 1, 1, 0, 0.835 } } );

  FilterRoutingListParameter list2;

  list2.addRouting( obj1 );

  BOOST_CHECK( list2.size() == 1 );

  list2.addRouting( obj2 );

  BOOST_CHECK( list2.size( ) == 2 );

  bool removeRes = list2.removeRouting( obj1.inputIndex, obj1.outputIndex );

  BOOST_CHECK( removeRes and list2.size( ) == 1 );

  removeRes = list2.removeRouting( obj1.inputIndex, obj1.outputIndex );

  BOOST_CHECK( (not removeRes) and list2.size( ) == 1 );

}

BOOST_AUTO_TEST_CASE( FilterRoutingParameterFromJson )
{
  std::string const jsonString = "[ { \"input\": 0, \"output\": 12, \"filter\": 8, \"gain\": 0.375 },{ \"input\": 3, \"output\": 1, \"filter\": 5} ]";

  FilterRoutingListParameter list1;

  BOOST_CHECK_NO_THROW( list1.parseJson( jsonString ) );

  BOOST_CHECK( list1.size() == 2 );
}

BOOST_AUTO_TEST_CASE( FilterRoutingParameterFromJsonMultiInput )
{
  std::string const initStr( "[ { \"input\": \"0:2:10\", \"output\": 12, \"filter\": 8, \"gain\": 0.375 } ]" );

  FilterRoutingListParameter list;

  BOOST_CHECK_NO_THROW( list.parseJson( initStr ) );

  BOOST_CHECK( list.size( ) == 6 );
}

BOOST_AUTO_TEST_CASE( FilterRoutingParameterFromJsonMultiInOut )
{
  std::string const initStr( "[ { \"input\": \"0:2:10\", \"output\": \"12,11,10,9,8,7\", \"filter\": 8, \"gain\": 0.375 } ]" );

  FilterRoutingListParameter list;

  BOOST_CHECK_NO_THROW( list.parseJson( initStr ) );

  BOOST_CHECK( list.size( ) == 6 );
}

BOOST_AUTO_TEST_CASE( FilterRoutingParameterFromJsonMultiInOutFilterGain )
{
  std::string const initStr( "[ { \"input\": \"0:2\", \"output\": \"7:-1:5\", \"filter\": \"3,8,2\", \"gain\": \"0.1, 0.25, 0.825\" } ]" );

  FilterRoutingListParameter list;

  BOOST_CHECK_NO_THROW( list.parseJson( initStr ) );

  BOOST_CHECK( list.size( ) == 3 );
}

} // namespace test
} // namespace pml
} // namespace visr
