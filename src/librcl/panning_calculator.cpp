/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "panning_calculator.hpp"

#include <libvisr/status_message.hpp>

#include <libefl/basic_matrix.hpp>
#include <libefl/cartesian_spherical_conversion.hpp>
#include <libefl/degree_radian_conversion.hpp>
#include <libefl/matrix_functions.hpp>
#include <libefl/vector_functions.hpp>

#include <libobjectmodel/object_vector.hpp>
#include <libobjectmodel/diffuse_source.hpp>
#include <libobjectmodel/point_source.hpp>
#include <libobjectmodel/point_source_with_diffuseness.hpp>
#include <libobjectmodel/plane_wave.hpp>

#include <libpanning/LoudspeakerArray.h>

#include <libpml/listener_position.hpp>
#include <libpml/shared_data_protocol.hpp>
#include <libpml/matrix_parameter.hpp>
#include <libpml/object_vector.hpp>
#include <libpml/double_buffering_protocol.hpp>


#include <algorithm>
#include <ciso646>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <numeric>

// for math utility functions (see implementations in the unnamed namespace below)

namespace visr
{
namespace rcl
{

  PanningCalculator::PanningCalculator( SignalFlowContext const & context,
                                        char const * name,
                                        CompositeComponent * parent,
                                        std::size_t numberOfObjects,
                                        panning::LoudspeakerArray const & arrayConfig,
                                        bool adaptiveListenerPosition /*= false*/,
                                        bool separateLowpassPanning /*= false*/ )
 : AtomicComponent( context, name, parent )
 , mHighFrequencyGains(cVectorAlignmentSamples )
 , mNumberOfObjects( numberOfObjects )
 , mNumberOfRegularLoudspeakers( arrayConfig.getNumRegularSpeakers() )
 , mNumberOfAllLoudspeakers( arrayConfig.getNumSpeakers() )
 , mLoudspeakerPositions( mVectorDimension, mNumberOfAllLoudspeakers )
 , mVectorDimension( arrayConfig.is2D() ? 2 : 3 )
 , mTmpGains( mNumberOfRegularLoudspeakers, cVectorAlignmentSamples )
  {
    // This contains all loudspeakers.
    for( std::size_t lspIdx( 0 ); lspIdx < mNumberOfAllLoudspeakers; ++lspIdx )
    {
      panning::XYZ pos = arrayConfig.getPosition( lspIdx );
      pos.normalise();
      mLoudspeakerPositions( 0, lspIdx ) = pos.x;
      mLoudspeakerPositions( 1, lspIdx ) = pos.y;
      if( mVectorDimension > 2 )
      {
        mLoudspeakerPositions( 2, lspIdx ) = pos.z;
      }
    }

    // Compute the triplet centers
    efl::BasicMatrix<CoefficientType> tripletCenters( mVectorDimension, arrayConfig.getNumTriplets() );
    std::vector<bool> tripletProcessed( arrayConfig.getNumTriplets(), false );

    mVbapCalculator.reset( new panning::VBAP( arrayConfig ) );

    // set the default initial listener position. This also initialises the internal data members (e.g. inverse matrices)
    setListenerPosition( static_cast<CoefficientType>(0.0), static_cast<CoefficientType>(0.0), static_cast<CoefficientType>(0.0) );

    mObjectVectorInput.reset( new ObjectPort( "objectVectorInput", *this, pml::EmptyParameterConfig() ) );
    mGainOutput.reset( new MatrixPort( "gainOutput", *this, pml::MatrixParameterConfig( mNumberOfRegularLoudspeakers, mNumberOfObjects ) ) );

    if( adaptiveListenerPosition )
    {
      mListenerPositionInput.reset( new ListenerPositionPort( "listenerPosition", *this, pml::EmptyParameterConfig() ) );
    }

    if( separateLowpassPanning )
    {
      mLowFrequencyGainOutput.reset( new MatrixPort( "lowFrequencyGainOutput", *this,
        pml::MatrixParameterConfig( mNumberOfRegularLoudspeakers, mNumberOfObjects ) ) );
      // What to do with virtual loudspeakers? At the moment, their energy is discarded. Thus computing a normalisation factor afterwards without incorporating them could
      // result in a very low value, boosting the gains of the real loudspeakers.
      // TODO: Reconsider after another strategy has been selected for handling virtual loudspeakers (downmix matrices before normalisation?)
      mHighFrequencyGains.resize( mNumberOfAllLoudspeakers );
    }
}

PanningCalculator::~PanningCalculator()
{
}

void PanningCalculator::setListenerPosition( CoefficientType x, CoefficientType y, CoefficientType z )
{
  mVbapCalculator->setListenerPosition( x, y, z );
}

void PanningCalculator::setListenerPosition( pml::ListenerPosition const & pos )
{
  setListenerPosition( pos.x(), pos.y(), pos.z() );
}

void PanningCalculator::process()
{
  bool const listenerPosChanged = mListenerPositionInput && mListenerPositionInput->changed();
  if( listenerPosChanged )
  {
    setListenerPosition( mListenerPositionInput->data( ) );
    mListenerPositionInput->resetChanged( );
  }

  if( mObjectVectorInput->changed() )
  {
    objectmodel::ObjectVector const & objects = mObjectVectorInput->data( );

    pml::MatrixParameter<CoefficientType> & gainMatrix = mGainOutput->data( );

    if( (gainMatrix.numberOfRows( ) != mNumberOfRegularLoudspeakers) or( gainMatrix.numberOfColumns( ) != mNumberOfObjects ) )
    {
      throw std::invalid_argument( "PanningCalculator::process(): The size of the gain matrix does not match the object/loudspeaker configuration." );
    }
    // Not necessary if we opt to set the complete matrix.
    gainMatrix.zeroFill( );

    // For the moment, we assume that the audio channels of the objects are identical to the final channel numbers.
    // Any potential re-routing will be added later.
    for( objectmodel::Object const & obj : objects )
    {
      if( obj.numberOfChannels( ) != 1 )
      {
        std::cerr << "PanningCalculator: Only monaural object types are supported at the moment." << std::endl;
        continue;
      }

      objectmodel::Object::ChannelIndex const channelId = obj.channelIndex( 0 );
      if( channelId >= mNumberOfObjects )
      {
        std::cerr << "PanningCalculator: Channel index \"" << channelId << "\" of object id#" << obj.id()
            << "exceeds number of channels (" << mNumberOfObjects << ")." << std::endl;
        continue;
      }

      // Use C++ type information to check whether the source is a pointsource.
      objectmodel::PointSource const * pointSrc = dynamic_cast<objectmodel::PointSource const *>(&obj);
      if( pointSrc )
      {
        // channelLock processing first
        std::vector<SampleType> sourcePos( mVectorDimension );
        sourcePos[0] = pointSrc->x();
        sourcePos[1] = pointSrc->y();
        if( mVectorDimension == 3 )
        {
          sourcePos[2] = pointSrc->z();
        }

        efl::ErrorCode res = efl::product( &sourcePos[0], mLoudspeakerPositions.data(), mTmpGains.data(),
                                           1 /*numResultRows*/, mNumberOfRegularLoudspeakers /* numResultColumns */, mVectorDimension /*numOp1Columns*/,
                                           1 /* op1RowStride */, 1 /*op1ColumnStride*/,
                                           mLoudspeakerPositions.stride() /*op2RowStride*/, 1 /*op2ColumnStride*/,
                                           1 /*resRowStride, not used, since there is only one row. */, 1 /*resColumnStride*/ );
        if( res != efl::noError )
        {
          // TODO: Replace by error API call.
          throw std::runtime_error( "PanningCalculator: dot product for computing channel lock failed.");
        }
        // Search only across the regular loudspeakers
        // We search for the maximum, because we are using the dot product, i.e., the cosine of the angular distance.
        // In case of equidistant nearest speakers, the first is returned.
        CoefficientType const * maxDotProduct = std::max_element( mTmpGains.data(), mTmpGains.data() + mNumberOfRegularLoudspeakers );
        if( maxDotProduct - mTmpGains.data() >= static_cast<std::ptrdiff_t>(mNumberOfRegularLoudspeakers) )
        {
          // TODO: Replace by error API call.
          throw std::runtime_error( "PanningCalculator: dot product for computing channel lock failed.");
        }
        CoefficientType const lockLimit = std::cos( efl::degree2radian(pointSrc->channelLockDistance() ) );
        if( *maxDotProduct >= lockLimit )
        {
          // Cast is safe because maxDotProduct >= mLoudspeakerDotProducts.data() always holds because of the way it is obtained
          std::size_t const lockLspIdx = static_cast<std::size_t>( maxDotProduct - mTmpGains.data() );
          mTmpGains.zeroFill();
          mTmpGains[lockLspIdx] = static_cast<SampleType>(1.0);
        }
        else
        {
          mVbapCalculator->calculateGains( pointSrc->x(), pointSrc->y(), pointSrc->z(),
                                           mTmpGains.data() );
        }
        // special handling for point sources with diffuseness: Adjust the level of the direct sound.
        objectmodel::PointSourceWithDiffuseness const * psdSrc = dynamic_cast<objectmodel::PointSourceWithDiffuseness const *>(&obj);
        if( psdSrc )
        {
          efl::ErrorCode res = efl::vectorMultiplyConstantInplace( 1.0f-psdSrc->diffuseness(), mTmpGains.data(), mNumberOfRegularLoudspeakers );
          if( res != efl::noError )
          {
            status( StatusMessage::Error, "Gain adjustment for source diffuseness failed: ", efl::errorMessage(res) );
            return;
          }
        }
      }
      else
      {
        objectmodel::PlaneWave const * pwSrc = dynamic_cast<objectmodel::PlaneWave const *>(&obj);
        {
          if( pwSrc )
          {
            SampleType posX, posY, posZ;
            std::tie( posX, posY, posZ )
            = efl::spherical2cartesian( efl::degree2radian( pwSrc->incidenceAzimuth() ),
                                        efl::degree2radian( pwSrc->incidenceElevation() ), 1.0f );
            mVbapCalculator->calculateGains( posX, posY, posZ, mTmpGains.data() );
            // We need to copy the data explicitly into a matrix column of a row-major matrix.
            // This could be replaced by a copy function with a stride argument.
            efl::ErrorCode res = efl::vectorCopyStrided( mTmpGains.data(), &gainMatrix( 0, channelId ), 1, gainMatrix.stride(),
                                                         mNumberOfRegularLoudspeakers, 0/*no assumptions about alignment possible*/ );
            if( res != efl::noError )
            {
              status( StatusMessage::Error, "Error while copying panning gains: ", efl::errorMessage(res) );
              return;
            }
          }
        }
      }
      if( separateLowpassPanning() ) // If there
      {
        // First compute the LF gains by re-normalising to the coherent sound pressure (sum of gains)
        // In case of nonnegative gains this is equivalent to the l1 norm.
        // If this cannot be ensured, we need to decide whether we want to normalise for the l1 norm or the sound pressure.
        SampleType const sum = std::accumulate( mTmpGains.data(), mTmpGains.data()+mNumberOfRegularLoudspeakers,
                                                static_cast<SampleType>(0.0),
                                                [](SampleType acc, SampleType val ){ return acc + val; } );
        SampleType const scaleFactor = static_cast<SampleType>(1.0) / sum;
        pml::MatrixParameter<SampleType> & lfGains = mLowFrequencyGainOutput->data();
        for( std::size_t lspIdx(0); lspIdx < mNumberOfRegularLoudspeakers; ++lspIdx )
        {
          lfGains( lspIdx, channelId ) = scaleFactor * mTmpGains[lspIdx];
        }

        // Now compute the VBIP (HF) gains
        std::for_each( mTmpGains.data(), mTmpGains.data()+mNumberOfRegularLoudspeakers,
                       // Safeguard against potentially negative values that would yield NaNs
                       [](SampleType & val ){ val = std::sqrt( std::max( val, static_cast<SampleType>(0.0) ) ); });
        // Re-normalise with l2 (power) norm.
        SampleType const l2Sqr = std::accumulate( mTmpGains.data(), mTmpGains.data()+mNumberOfRegularLoudspeakers,
                                                  static_cast<SampleType>(0.0),
                                                  [](SampleType acc, SampleType val ){ return acc + val*val; } );
        // l2Sqr should be identical to 'sum' provided that the gains are nonnegative.
        SampleType const l2ScaleFactor = static_cast<SampleType>(1.0)/ std::sqrt(l2Sqr);
        std::for_each( mTmpGains.data(), mTmpGains.data()+mNumberOfRegularLoudspeakers,
                       [l2ScaleFactor](SampleType & val ){ val = l2ScaleFactor * val; } );
        // Fall through to copy the HF gains
      }
      //  We need to copy the data explicitly into a matrix column of a row-major matrix.
      efl::ErrorCode const res = efl::vectorCopyStrided( mTmpGains.data(), &gainMatrix( 0, channelId ), 1, gainMatrix.stride(),
                                                         mNumberOfRegularLoudspeakers, 0/*no assumptions about alignment possible*/ );
      if( res != efl::noError )
      {
        status( StatusMessage::Error, "Error while copying panning gains: ", efl::errorMessage(res) );
        return;
      }
    }
    mObjectVectorInput->resetChanged();
  }
}

} // namespace rcl
} // namespace visr
