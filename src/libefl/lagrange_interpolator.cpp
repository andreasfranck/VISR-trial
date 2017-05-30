/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "lagrange_interpolator.hpp"

#include <algorithm>
#include <cassert>

namespace visr
{
namespace efl
{

#ifdef LAGRANGE_INTERPOLATOR_GENERATE_STAGE_TABLES
template< typename DataType, std::size_t N>
/*static*/ const std::array<std::size_t, LagrangeInterpolator<DataType,N>::numberOfStages()>
LagrangeInterpolator<DataType,N>::cStageSizes{ LagrangeInterpolator<DataType,N>::generateStageSizes() };

template< typename DataType, std::size_t N>
/*static*/ const std::array<std::size_t, LagrangeInterpolator<DataType,N>::numberOfStages()>
LagrangeInterpolator<DataType,N>::cStageOffsets( LagrangeInterpolator<DataType,N>::generateStageOffsets() );
#endif

template< typename DataType, std::size_t N, bool reverseCoeffs >
LagrangeInterpolator<DataType,N, reverseCoeffs>::LagrangeInterpolator()
  : cOffsets( generateOffsets() )
  , cScaleFactors( generateScaleFactors() )
{

}

template< typename DataType, std::size_t N, bool reverseCoeffs >
template<std::size_t length>
void LagrangeInterpolator< DataType, N, reverseCoeffs>::multiplyAndShuffle( DataType * coeffs ) const
{
  if( length > 2 )
  {
    static constexpr bool oddLength = (length % 2) != 0;
    static constexpr std::size_t arrayLength = (length + 1) / 2;
    std::array<DataType, arrayLength> prod;
    static std::size_t constexpr pairs = length / 2;
    for( std::size_t idx( 0 ); idx < pairs; ++idx )
    {
      prod[idx] = coeffs[2 * idx] * coeffs[2 * idx + 1];
    }
    if( oddLength )
    {
      prod[pairs] = coeffs[2 * pairs];
    }
    multiplyAndShuffle<(length + 1) / 2>( &prod[0] );
    for( std::size_t idx( 0 ); idx < pairs; ++idx )
    {
      DataType const tmp = coeffs[2 * idx + 1] * prod[idx];
      coeffs[2 * idx + 1] = coeffs[2 * idx] * prod[idx];
      coeffs[2 * idx] = tmp;
    }
    if( oddLength )
    {
      coeffs[length - 1] = prod[pairs];
    }
  }
  else if( length == 2 )
  {
    std::swap( coeffs[0], coeffs[1] );
    return;
  }
  else if( length == 1 )
  {
    coeffs[0] = static_cast<DataType>(0.0f);
  }
  else
  {
    assert( false );
  }
}

//template< typename DataType, std::size_t N>
//template<>
//inline void LagrangeInterpolator< DataType, N>::multiplyAndShuffle<2>( DataType * coeffs ) const
//{
//  std::swap( coeffs[0], coeffs[1] );
//}


template< typename DataType, std::size_t N, bool reverseCoeffs >
void LagrangeInterpolator<DataType, N, reverseCoeffs>::
calculateCoefficients( DataType mu, std::array<DataType,N+1> & result ) const
{
  std::transform( cOffsets.begin(), cOffsets.end(), mInternalCoeffs.begin(), [mu]( DataType val ) { return val - mu; } );

  multiplyAndShuffle< N+1 >( &mInternalCoeffs[0] );

  std::transform( cScaleFactors.begin(), cScaleFactors.end(), mInternalCoeffs.begin(), result.begin(), 
    []( DataType lhs, DataType rhs ) { return lhs * rhs; } );
}

template< typename DataType, std::size_t N, bool reverseCoeffs>
/*static*/ DataType LagrangeInterpolator<DataType,N, reverseCoeffs>::offset( std::size_t idx )
{
  return reverseCoeffs 
   ? static_cast<DataType>(0.5)*(static_cast<DataType>(N)) - static_cast<DataType>(idx)
   : static_cast<DataType>(idx) - static_cast<DataType>(0.5)*(static_cast<DataType>(N));
}

template< typename DataType, std::size_t N, bool reverseCoeffs>
/*static*/ DataType LagrangeInterpolator<DataType, N, reverseCoeffs>::scaleFactor( std::size_t idx )
{
  DataType const currOffset = offset(idx);
  DataType acc{ static_cast<DataType>(1.0) };
  for( std::size_t runIdx(0); runIdx <= N; ++runIdx )
  {
    if( runIdx != idx )
    {
      acc *= offset(runIdx) - currOffset;
    }
  }
  return static_cast<DataType>(1.0)/static_cast<DataType>(acc);
}

template< typename DataType, std::size_t N, bool reverseCoeffs>
/*static*/ std::array<DataType, N+1> LagrangeInterpolator<DataType,N, reverseCoeffs>::generateOffsets()
{
  std::array<DataType, N+1> offsets;
  for( std::size_t idx(0); idx <= N; ++idx )
  {
    offsets[idx] = offset(idx);
  }
  return offsets;
}

template< typename DataType, std::size_t N, bool reverseCoeffs>
/*static*/ std::array<DataType, N+1> LagrangeInterpolator<DataType,N, reverseCoeffs>::generateScaleFactors()
{
  std::array<DataType, N+1> scaleFactors;
  for( std::size_t idx(0); idx <= N; ++idx )
  {
    scaleFactors[idx] =  scaleFactor(idx);
  }
  return scaleFactors;
}

#ifdef LAGRANGE_INTERPOLATOR_GENERATE_STAGE_TABLES
template< typename DataType, std::size_t N>
std::array<std::size_t, LagrangeInterpolator<DataType, N>::numberOfStages()> visr::efl::LagrangeInterpolator<DataType, N>::generateStageSizes()
{
  std::array<std::size_t, LagrangeInterpolator<DataType,N>::numberOfStages()> stageSizes;
  for( std::size_t idx(0); idx < numberOfStages(); ++idx )
  {
    stageSizes[idx] =  stageSize(idx);
  }
  return stageSizes;
}


template< typename DataType, std::size_t N>
/*static*/ std::array<std::size_t, LagrangeInterpolator<DataType,N>::numberOfStages()> LagrangeInterpolator<DataType,N>::generateStageOffsets()
{
  std::array<size_t, LagrangeInterpolator<DataType,N>::numberOfStages()> stageSizes;
  for( std::size_t idx(0); idx < numberOfStages(); ++idx )
  {
    stageSizes[idx] =  stageSize(idx);
  }
  return stageSizes;
}
#endif

// Explicit instantiations
template class LagrangeInterpolator<float, 0>; // Valid, but not very sensible way to implement a zeroth-order (neares neighbour) interpolation.
template class LagrangeInterpolator<float, 1>;
template class LagrangeInterpolator<float, 2>; // Useless but supported
template class LagrangeInterpolator<float, 3>;
template class LagrangeInterpolator<float, 4>; // Useless but supported
template class LagrangeInterpolator<float, 5>;
template class LagrangeInterpolator<float, 6>; // Useless but supported
template class LagrangeInterpolator<float, 7>;
template class LagrangeInterpolator<float, 8>; // Useless but supported
template class LagrangeInterpolator<float, 9>;

// Same for reverted coefficients.
template class LagrangeInterpolator<float, 0, true>; // Valid, but not very sensible way to implement a zeroth-order (neares neighbour) interpolation.
template class LagrangeInterpolator<float, 1, true>;
template class LagrangeInterpolator<float, 2, true>; // Useless but supported
template class LagrangeInterpolator<float, 3, true>;
template class LagrangeInterpolator<float, 4, true>; // Useless but supported
template class LagrangeInterpolator<float, 5, true>;
template class LagrangeInterpolator<float, 6, true>; // Useless but supported
template class LagrangeInterpolator<float, 7, true>;
template class LagrangeInterpolator<float, 8, true>; // Useless but supported
template class LagrangeInterpolator<float, 9, true>;


} // namespace efl
} // namespace visr
