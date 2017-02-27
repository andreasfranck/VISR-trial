/* Copyright Institue of Sound and Vibration Research - All rights reserved. */

#include <librrl/audio_signal_flow.hpp>

#include <libril/audio_input.hpp>
#include <libril/audio_output.hpp>
#include <libril/atomic_component.hpp>
#include <libril/composite_component.hpp>
#include <libril/signal_flow_context.hpp>

#include <boost/test/unit_test.hpp>

#include <ciso646>
#include <cstddef>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

namespace visr
{
namespace rrl
{
namespace test
{

namespace // unnamed
{

static std::size_t const audioWidth = 1;

std::string numberedItem( std::string const & base, std::size_t index )
{
  std::stringstream res;
  res << base << "_" << index;
  return res.str();
}

class MyAtom: public AtomicComponent
{
public:
  MyAtom( SignalFlowContext & context, char const * componentName, CompositeComponent * parent,
          std::size_t numInputs, std::size_t numOutputs)
   : AtomicComponent( context, componentName, parent )
  {
    for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
    {
      mInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "in", portIdx ).c_str( ), *this ) ) );
      mInputs[portIdx]->setWidth( audioWidth );
    }
    for( std::size_t portIdx( 0 ); portIdx < numOutputs; ++portIdx )
    {
      mOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "out", portIdx ).c_str( ), *this ) ) );
      mOutputs[portIdx]->setWidth( audioWidth );
    }
   }
  void process() override {}
private:
  std::vector< std::unique_ptr<AudioInput> > mInputs;
  std::vector<std::unique_ptr<AudioOutput> > mOutputs;

};

class MyComposite: public CompositeComponent
{
public:
  MyComposite( SignalFlowContext & context, char const * componentName, CompositeComponent * parent,
               std::size_t numInputs, std::size_t numOutputs )
    : CompositeComponent( context, componentName, parent )
    , mAtom( context, "SecondLevelAtom", this, numInputs, numOutputs )
  {
    ChannelRange const indices( 0, audioWidth );

    for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
    {
      mExtInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "placeholder_in", portIdx ).c_str(), *this ) ) );
      mExtInputs[portIdx]->setWidth( audioWidth );
      registerAudioConnection( "this", numberedItem( "placeholder_in", portIdx ), indices,
                               "SecondLevelAtom", numberedItem( "in", portIdx ), indices );

    }
    for( std::size_t portIdx( 0 ); portIdx < numOutputs; ++portIdx )
    {
      mExtOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "placeholder_out", portIdx ).c_str(), *this ) ) );
      mExtOutputs[portIdx]->setWidth( audioWidth );
      registerAudioConnection( "SecondLevelAtom", numberedItem( "out", portIdx ), indices,
                               "this", numberedItem( "placeholder_out", portIdx ), indices );
    }
  }
private:
  MyAtom mAtom;

  std::vector< std::unique_ptr<AudioInput> > mExtInputs;
  std::vector<std::unique_ptr<AudioOutput> > mExtOutputs;
};

class MyRecursiveComposite: public CompositeComponent
{
public:
  MyRecursiveComposite( SignalFlowContext & context, char const * componentName, CompositeComponent * parent,
                        std::size_t numInputs, std::size_t numOutputs, std::size_t recursionCount, bool insertAtom )
    : CompositeComponent( context, componentName, parent )
  {
    if( (not insertAtom) and( numInputs != numOutputs ) )
    {
      throw std::invalid_argument( "If \"insertAtom\" is false, then the number of inputs must match the number of outputs." );
    }
    ChannelRange const indices( 0, audioWidth, 1 );
    std::stringstream childNameStr;
    if( (recursionCount > 0) or insertAtom )
    {
      if( recursionCount > 0 )
      {
        childNameStr << "level" << recursionCount-1 << "composite";
        mChild.reset( new MyRecursiveComposite( context, childNameStr.str().c_str(), this, numInputs, numOutputs, recursionCount - 1, insertAtom ) );
        for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
        {
          mExtInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "placeholder_in", portIdx ).c_str( ), *this ) ) );
          mExtInputs[portIdx]->setWidth( audioWidth );
          registerAudioConnection( "this", numberedItem( "placeholder_in", portIdx ), indices,
            childNameStr.str( ).c_str( ), numberedItem( "placeholder_in", portIdx ), indices );
        }
        for( std::size_t portIdx( 0 ); portIdx < numOutputs; ++portIdx )
        {
          mExtOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "placeholder_out", portIdx ).c_str( ), *this ) ) );
          mExtOutputs[portIdx]->setWidth( audioWidth );
          registerAudioConnection( childNameStr.str( ).c_str( ), numberedItem( "placeholder_out", portIdx ), indices,
            "this", numberedItem( "placeholder_out", portIdx ), indices );
        }
      }
      else
      {
        childNameStr << "level" << recursionCount << "atom";
        mChild.reset( new MyAtom( context, childNameStr.str( ).c_str( ), this, numInputs, numOutputs ) );
        for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
        {
          mExtInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "placeholder_in", portIdx ).c_str( ), *this ) ) );
          mExtInputs[portIdx]->setWidth( audioWidth );
          registerAudioConnection( "this", numberedItem( "placeholder_in", portIdx ), indices,
            childNameStr.str( ).c_str( ), numberedItem( "in", portIdx ), indices );
        }
        for( std::size_t portIdx( 0 ); portIdx < numOutputs; ++portIdx )
        {
          mExtOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "placeholder_out", portIdx ).c_str( ), *this ) ) );
          mExtOutputs[portIdx]->setWidth( audioWidth );
          registerAudioConnection( childNameStr.str( ).c_str( ), numberedItem( "out", portIdx ), indices,
            "this", numberedItem( "placeholder_out", portIdx ), indices );
        }
      }
    }
    else // no atom at the lowest level, just connect the composite input to the output
    {
      for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
      {
        mExtInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "placeholder_in", portIdx ).c_str( ), *this ) ) );
        mExtOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "placeholder_out", portIdx ).c_str( ), *this ) ) );
        mExtInputs[portIdx]->setWidth( audioWidth );
        mExtOutputs[portIdx]->setWidth( audioWidth );
        registerAudioConnection( "this", numberedItem( "placeholder_in", portIdx ), indices,
          "this", numberedItem( "placeholder_out", portIdx ), indices );
      }

    }
    }
private:
  std::unique_ptr<Component> mChild;

  std::vector< std::unique_ptr<AudioInput> > mExtInputs;
  std::vector<std::unique_ptr<AudioOutput> > mExtOutputs;
};


class MyTopLevel: public CompositeComponent
{
public:
  MyTopLevel( SignalFlowContext & context, char const * componentName, CompositeComponent * parent,
              std::size_t numInputs, std::size_t numOutputs )
   : CompositeComponent( context, componentName, parent )
   , mComposite1( context, "FirstLevelComposite", this, numInputs, numOutputs )
   , mAtomTopLevel( context, "FirstLevelAtom", this, numOutputs, numOutputs )
  {
     ChannelRange const indices( 0, audioWidth, 1 );

     for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
     {
       mExtInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "ext_in", portIdx ).c_str( ), *this ) ) );
       mExtInputs[portIdx]->setWidth( audioWidth );
       registerAudioConnection( "this", numberedItem( "ext_in", portIdx ), indices,
         "FirstLevelComposite", numberedItem( "placeholder_in", portIdx ), indices );

     }
     for( std::size_t portIdx( 0 ); portIdx < numOutputs; ++portIdx )
     {
       mExtOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "ext_out", portIdx ).c_str( ), *this ) ) );
       mExtOutputs[portIdx]->setWidth( audioWidth );
       registerAudioConnection( "FirstLevelComposite", numberedItem( "placeholder_out", portIdx ), indices,
         "FirstLevelAtom", numberedItem( "in", portIdx ), indices );
       registerAudioConnection( "FirstLevelAtom", numberedItem( "out", portIdx ), indices,
         "this", numberedItem( "ext_out", portIdx ), indices );
     }
  }
private:
  MyComposite mComposite1;
  MyAtom mAtomTopLevel;

  std::vector< std::unique_ptr<AudioInput> > mExtInputs;
  std::vector<std::unique_ptr<AudioOutput> > mExtOutputs;
};

class MyTopLevelRecursive: public CompositeComponent
{
public:
  MyTopLevelRecursive( SignalFlowContext & context, char const * componentName, CompositeComponent * parent,
    std::size_t numInputs, std::size_t numOutputs, std::size_t recursionLevels, bool insertAtom )
    : CompositeComponent( context, componentName, parent )
    , mComposite( context, "FirstLevelComposite", this, numInputs, numOutputs, recursionLevels, insertAtom )
  {
      ChannelRange const indices( 0, audioWidth );

      for( std::size_t portIdx( 0 ); portIdx < numInputs; ++portIdx )
      {
        mExtInputs.push_back( std::unique_ptr<AudioInput>( new AudioInput( numberedItem( "ext_in", portIdx ).c_str( ), *this ) ) );
        mExtInputs[portIdx]->setWidth( audioWidth );
        registerAudioConnection( "this", numberedItem( "ext_in", portIdx ), indices,
          "FirstLevelComposite", numberedItem( "placeholder_in", portIdx ), indices );

      }
      for( std::size_t portIdx( 0 ); portIdx < numOutputs; ++portIdx )
      {
        mExtOutputs.push_back( std::unique_ptr<AudioOutput>( new AudioOutput( numberedItem( "ext_out", portIdx ).c_str( ), *this ) ) );
        mExtOutputs[portIdx]->setWidth( audioWidth );
        registerAudioConnection( "FirstLevelComposite", numberedItem( "placeholder_out", portIdx ), indices,
          "this", numberedItem( "ext_out", portIdx ), indices );
      }
    }
private:
  MyRecursiveComposite mComposite;

  std::vector< std::unique_ptr<AudioInput> > mExtInputs;
  std::vector<std::unique_ptr<AudioOutput> > mExtOutputs;
};


} // namespace unnamed

BOOST_AUTO_TEST_CASE( CheckAtomicComponent )
{
  SignalFlowContext context( 1024, 48000 );

  MyAtom atomicComp( context, "", nullptr, 2, 3 );

  rrl::AudioSignalFlow flow( atomicComp );

  // Perform basic tests of the external I/O interface
}

BOOST_AUTO_TEST_CASE( CheckCompositeComponent )
{
  SignalFlowContext context( 1024, 48000 );

  MyComposite composite( context, "", nullptr, 2, 3 );

  rrl::AudioSignalFlow flow( composite );

  // Perform basic tests of the external I/O interface
}

BOOST_AUTO_TEST_CASE( CheckTwoLevelCompositeComponent )
{
  SignalFlowContext context( 1024, 48000 );

  MyTopLevel composite( context, "", nullptr, 3, 4 );

  rrl::AudioSignalFlow flow( composite );

  // Perform basic tests of the external I/O interface
}

/**
 * Check the instantiation of a recursive hierachical signal flow that hast a composite component at the lowest level which
 * interconnect the input to the output. The flattening algorithms results in a direct interconnection of the top-level capture 
 * to the top-level playback port.
 */
BOOST_AUTO_TEST_CASE( CheckRecursiveCompositeComponentNoAtom )
{
  SignalFlowContext context( 1024, 48000 );

  std::size_t const recursionLimit = 3;
  bool const insertAtom = false;

  MyTopLevelRecursive composite( context, "", nullptr, 4, 4, recursionLimit, insertAtom );

  rrl::AudioSignalFlow flow( composite );

  // Perform basic tests of the external I/O interface
}

/**
 * Check the instantiation of a recursive hierachical signal flow that instantiates a single component 
 * at the lowest level and interconnects the inputs and outputs through all composite levels.
 */
BOOST_AUTO_TEST_CASE( CheckRecursiveCompositeComponent )
{
  SignalFlowContext context( 1024, 48000 );

  std::size_t const recursionLimit = 3;
  bool const insertAtom = true;

  MyTopLevelRecursive composite( context, "", nullptr, 3, 4, recursionLimit, insertAtom );

  rrl::AudioSignalFlow flow( composite );

  // Perform basic tests of the external I/O interface
}


} // namespace test
} // namespace rrl
} // namespace visr
