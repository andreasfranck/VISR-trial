/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "late_reverb_filter_calculator.hpp"

#include <libefl/basic_vector.hpp>

#include <libpml/biquad_parameter.hpp>

#include <algorithm>
#include <array>
#include <ciso646>
#include <cmath>
#include <iostream>
#include <functional>
#include <random>

namespace visr
{
namespace reverbobject
{

namespace
{

/**
 * Filter a signal with a second-order IIR filter given as biquad coefficients.
 * @tparam SampleType The data type for the samples (a floating-point type)
 * @param input The input sequence, must hold \p numSamples elements
 * @param output Array to store the output signals, must provide space for at least \p numSamples values.
 * @param numSamples The number of samples to process.
 * @param pastInputs Two-element array storing the previus inputs $x[n-1]$ and $x[n-2]$ (in this descending order). Defaults to ${0.0, 0.0}$
 * @param initialState Two-element array storing the initial state, i.e., the previous outputs$y[n-1]$ and $y[n-2]$ (in this descending order). Defaults to ${0.0, 0.0}$
 * @todo Clean up and move to libefl.
 * @todo Rethink data type and orientation of \p pastInputs and \p initialState.
 */
template<typename SampleType>
efl::ErrorCode filterBiquad( SampleType const * const input, SampleType * const output, std::size_t numSamples,
                            rbbl::BiquadCoefficient<SampleType> const & iir, std::array<SampleType, 2> const & pastInputs = { {0.0f, 0.0f} }, std::array<SampleType, 2> const & initialState = { {0.0f, 0.0f} } )
{
  std::array<SampleType, 2> state( initialState);
  std::array<SampleType, 3> inputBuffer = { {0.0f, pastInputs[0], pastInputs[1]} };
  // TODO: your code here.
  for( std::size_t sampleIdx( 0 ); sampleIdx < numSamples; ++sampleIdx )
  {
    inputBuffer[0] = inputBuffer[1];
    inputBuffer[1] = inputBuffer[2];
    inputBuffer[2] = input[sampleIdx];
    SampleType const y = iir.b0()*inputBuffer[2] + iir.b1()*inputBuffer[1] + iir.b2 ()*inputBuffer[0] - iir.a1()*state[1] - iir.a2 ()*state[0];
    output[sampleIdx]=y;

    state[0] = state[1];
    state[1] = y;
  }

  return efl::noError;
}

// Explicit instantiations
template efl::ErrorCode filterBiquad( float const * const input, float * const output, std::size_t numSamples,
  rbbl::BiquadCoefficient<float> const & iir, std::array<float, 2> const &, std::array<float, 2> const & );
// Disabled explicit instantiation for double to avoid a compiler warning becuase this version is not used at the moment. Enable if required.
#if 0
template efl::ErrorCode filterBiquad( double const * const input, double * const output, std::size_t numSamples,
     rbbl::BiquadCoefficient<double> const & iir, std::array<double, 2> const &, std::array<double, 2> const &);
 #endif

/**
 * Hard-coded IIR coefficients for a fixed set of 9 octave bands: { 62.5 Hz, 125 Hz, 250 Hz, 500 Hz, 1 kHz, 2 kHz, 4 kHz, 8 kHz, 16 kHz }.
 * Created using Matlab script generateBandpassCoefficients()
 * TODO: Add biquad coefficient calculation library functions to the framework(based on RBJ's audio EQ cookbook formulas)
 * and calculate the filters on the fly.
 */
static const rbbl::BiquadCoefficientList<SampleType> cOctaveBandFilters={
 { 0.000016684780519f,  0.000033369561037f,  0.000016684780519f, -1.994164939377186f, 0.994231678499260f },
 { 0.005751740181735f,  0.000000000000000f, -0.005751740181735f, -1.988230335335583f, 0.988496519636531f },
 { 0.011437753858235f,  0.000000000000000f, -0.011437753858235f, -1.976065915069578f, 0.977124492283531f },
 { 0.022617292733236f,  0.000000000000000f, -0.022617292733236f, -1.950580111730489f, 0.954765414533529f },
 { 0.044237741487938f,  0.000000000000000f, -0.044237741487938f, -1.895171159793622f, 0.911524517024123f },
 { 0.084754185122791f,  0.000000000000000f, -0.084754185122791f, -1.768119139985760f, 0.830491629754419f },
 { 0.156456676130944f,  0.000000000000000f, -0.156456676130944f, -1.461059895326733f, 0.687086647738111f },
 { 0.272011461038663f,  0.000000000000000f, -0.272011461038663f, -0.727988538961337f, 0.455977077922675f },
 { 0.137825787997573f, -0.275651575995145f,  0.137825787997573f,  0.551303151990290f, 0.102606303980581f } };

} // unnamed namespace

LateReverbFilterCalculator::LateReverbFilterCalculator( SignalFlowContext const & context,
                                                        char const * name,
                                                        CompositeComponent * parent,
                                                        std::size_t numberOfObjects,
                                                        SampleType lateReflectionLengthSeconds,
                                                        std::size_t numLateReflectionSubBandLevels,
                                                        std::size_t maxUpdatesPerPeriod /*= 0*/ )
 : AtomicComponent( context, name, parent )
 , mAlignment( cVectorAlignmentSamples )
 , mNumberOfObjects( numberOfObjects )
 , mNumberOfSubBands( numLateReflectionSubBandLevels )
 // Disallow filterLength == 0, because the processing chain is not completely safe in this case.
 // TODO: Consider passing the filter length as a number of samples, because currently this adjustment has to be made also in the ReverbObjectRenderer,
 // which makes it prone to inconsistencies.
 , mFilterLength( static_cast<std::size_t>(std::max(std::ceil( lateReflectionLengthSeconds * samplingFrequency() ), 1.0f) ) )
 , mMaxUpdatesPerIteration( maxUpdatesPerPeriod == 0 ? mNumberOfObjects : maxUpdatesPerPeriod )
 , mSubBandNoiseSequences( numberOfObjects * mNumberOfSubBands, mFilterLength, mAlignment )
 , mSubbandInput( "subbandInput", *this, pml::EmptyParameterConfig( ) )
 , mFilterOutput( "lateFilterOutput", *this, pml::EmptyParameterConfig( ) )
{
  std::size_t const numberOfExtraSamples = efl::nextAlignedSize( 20, mAlignment); // extra samples at the beginning of the noise
  // sequence to avoid any 'startup behaviour' of the IIR filter. The alignment magic is to allow us to use an aligned copy
  // operation to store the cropped result of the filtering.
  std::size_t const noiseLength = mFilterLength + numberOfExtraSamples;
  efl::BasicVector<SampleType> noiseSequence( noiseLength, mAlignment );
  efl::BasicVector<SampleType> filteredSequence( noiseLength, mAlignment );

  if( cOctaveBandFilters.size() != mNumberOfSubBands )
  {
    throw std::invalid_argument( "LateReverbFilterFilterCalculator: The number of subbands does not match the hard-coded IIR filter bank." );
  }

  for( std::size_t objIdx( 0 ); objIdx < mNumberOfObjects; ++objIdx )
  {
    for( std::size_t bandIdx( 0 ); bandIdx < mNumberOfSubBands; ++bandIdx )
    {
      createWhiteNoiseSequence( noiseSequence.size(), noiseSequence.data(), mAlignment );

      // filterBiquad( SampleType const * const input, SampleType * const output, std::size_t numSamples,
      // rbbl::BiquadCoefficient<SampleType> const & iir, std::array<SampleType, 2> const & pastInputs = { 0.0f, 0.0f }, std::array<SampleType, 2> const & initialState = { 0.0f, 0.0f } )
      filterSequence( noiseLength, noiseSequence.data( ), filteredSequence.data( ), cOctaveBandFilters.at(bandIdx ) );

      // Copy the last mFilterLength samples to the right position in the matrix.
      if( efl::vectorCopy( filteredSequence.data() + numberOfExtraSamples,
                           subBandNoiseSequence( objIdx, bandIdx ), mFilterLength,
                           mAlignment ) != efl::noError )
      {
        throw std::runtime_error( "LateReverbFilterCalculator::setup(): Copying of subband noise sequence failed." );
      }
    }
  }
}

LateReverbFilterCalculator::~LateReverbFilterCalculator()
{
}

void LateReverbFilterCalculator::process( )
{
  std::size_t objCnt = 0;
  while( not mSubbandInput.empty() )
  {
    LateReverbParameter const & val = mSubbandInput.front();
    if( val.index() >= mNumberOfObjects )
    {
      throw std::out_of_range( "LateReverbFilterCalculator: Object index out of range." );
    }
    std::vector<SampleType> newFilter( mFilterLength );

    // As the check for parameter changes is done on the sending end, we do not need to do it here again
    // Anyway, this will not change the impulse responses, as the calculation is deterministic.

    // Although we allow mFilterLength == 0 (corresponding to 'no late reverb') here, the access 
    // &newFilter[0] would be illegal and possibly triggers a debug assertion.
    // Note: Currently, the constructor adjusts this value to >=1
    if( mFilterLength > 0 )
    {
      calculateImpulseResponse(val.index(), val.getReverbParameters(), &newFilter[0], mFilterLength);
    }

    mFilterOutput.enqueue( IndexedFilter( val.index(), newFilter ) );

    mSubbandInput.pop();
    ++objCnt;
    if( objCnt >= mMaxUpdatesPerIteration )
    {
      break;
    }
  }
}

void LateReverbFilterCalculator::
calculateImpulseResponse( std::size_t objectIdx,
                          objectmodel::PointSourceWithReverb::LateReverb const & lateParams,
                          SampleType * ir,
                          std::size_t irLength, std::size_t alignment /*= 0*/ )
{
  try
  {
    std::size_t const finalAlignment = std::min( mAlignment, alignment );
    if( irLength < mFilterLength )
    {
      throw std::runtime_error( "LateReverbFilterCalculator::calculateImpulseResponse(): the passed filter buffer is too short.");
    }
    // Could be a member allocated in setup.
    efl::BasicVector<SampleType> envelope( mFilterLength, mAlignment );

    // Do whatever needed to calculate the reverb filter
    // for each subband.
    for( std::size_t subBandIdx= 0; subBandIdx < mNumberOfSubBands; ++subBandIdx )
    {
      // Create an envelope.
      createEnvelope( mFilterLength, envelope.data(), lateParams.onsetDelay(),
                      lateParams.levels()[subBandIdx], lateParams.attackTimes()[subBandIdx],
                      lateParams.decayCoeffs()[subBandIdx], static_cast<SampleType>(samplingFrequency()) );

      // multiply the sequence with the envelope and add them together
      efl::ErrorCode res;
      if( subBandIdx == 0 )
      {
        // Just a multiply for the zeroth subband filter to clear any previous content.
        res = efl::vectorMultiply( subBandNoiseSequence( objectIdx, subBandIdx ), envelope.data(),
                                   ir, mFilterLength, finalAlignment );
      }
      else
      {
        res = efl::vectorMultiplyAddInplace( subBandNoiseSequence( objectIdx, subBandIdx ), envelope.data( ),
                                             ir, mFilterLength, finalAlignment );
      }
      if( res != efl::noError )
      {
        throw std::runtime_error( "ReverbParameterCalculator::calculateImpulseResponse(): Calculation of final envelope failed." );
      }
    }
    // Zero any remaining zeros in the buffer
    if( irLength > mFilterLength )
    {
      if( efl::vectorZero( ir + mFilterLength, irLength > mFilterLength, 0 /* no alignment spec possible */ ) != efl::noError )
      {
        throw std::runtime_error( "ReverbParameterCalculator::calculateImpulseResponse(): Zeroing of remaining ir taps failed." );
      }
    }
  }
  catch( std::exception const & ex )
  {
    std::cerr << "Error while calculating late reverberation impulse response: " << ex.what() << ", skipping calculation." << std::endl;
  }
}

/**
* Create a uniform white noise sequence with range [-1,1].
* @param numSamples Length of the noise sequence.
* @param [out] data Buffer to store the result.
* @param alignment Alignment of the \p data buffer (in number of elements)
*/
/*static*/ void LateReverbFilterCalculator::createWhiteNoiseSequence( std::size_t numSamples,
                                                                      SampleType* data,
                                                                      std::size_t alignment )
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<SampleType> dis(-1.0f, 1.0f);

  std::generate( data, data + numSamples, std::bind( dis, gen ) );
}

/*static*/ void LateReverbFilterCalculator::filterSequence( std::size_t numSamples, SampleType const * const input, SampleType * output,
  rbbl::BiquadCoefficient<SampleType> const & filter )
{

  filterBiquad(input, output, numSamples, filter);

}


/*static*/ void LateReverbFilterCalculator::createEnvelope( std::size_t numSamples, SampleType* data,
                                                            SampleType initialDelay, SampleType gain,
                                                            SampleType attackCoeff, SampleType decayCoeff,
                                                            SampleType samplingFrequency )
{
  // initialDelay in seconds sets leading zeros
  // attackCoeff in seconds is the length of the onset ramp
  // decayCoeff is the decay constant for the exponential (late) decay

  std::size_t const initialDelaySamples = static_cast<std::size_t>(std::round( initialDelay*samplingFrequency ));
  std::size_t attackCoeffSamples = static_cast<std::size_t>(std::round( attackCoeff*samplingFrequency ));
  SampleType decayCoeffSamples = decayCoeff/static_cast<SampleType>(samplingFrequency);
  
  if( initialDelaySamples >= numSamples || initialDelaySamples+attackCoeffSamples >= numSamples )
  {
    throw std::invalid_argument( "The late reverberation envelope exceeds the maximum number of samples" );
  }

  for( std::size_t n = 0; n < initialDelaySamples; ++n )
  { // leading zeros for delay
        data[n]=0.0f;
  }
  for( std::size_t n = initialDelaySamples; n < attackCoeffSamples + initialDelaySamples; ++n )
  { // linear increase up to max
    data[n] = gain * (static_cast<SampleType>(n - initialDelaySamples) / static_cast<SampleType>(attackCoeffSamples));
  }
  for( std::size_t n = attackCoeffSamples + initialDelaySamples; n < numSamples; ++n )
  { // exponential decay to end of envelope
    SampleType const decay = gain * std::exp( decayCoeffSamples*(n - attackCoeffSamples - initialDelaySamples) );
    data[n] = decay;
  }
}
  
} // namespace reverbobject
} // namespace visr
