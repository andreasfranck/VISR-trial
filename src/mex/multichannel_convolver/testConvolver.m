

bs = 64;
numBlocks = 64;
inputLength = bs * numBlocks;

fs =48e3;

t = (0:inputLength-1)/fs;

input = [ sin( 2*pi * 880 * t); 0.5*cos( 2*pi* 2345 *t )].';

numberOfOutputs = 4;

filterLength = 1024; 

numFilters = 4;

maxFilters = 8;
maxRoutings = 6;

filters = zeros( numFilters, filterLength );
filters( 1, 1 ) = 1; % unit Dirac impulse;
filters( 2, 16 ) = -0.5; % delayed and scaled Dirac;
filters( 3, : ) = fir1( filterLength-1, 0.3 );
filters( 4, 1:end-1 ) = fir1( filterLength-2, 0.3, 'high');

routings = ...
[ 0, 0, 1.0, 0;...
  1, 0, -2.0, 1;...
  0, 1, 0.7, 2;...
  0, 2, 0.8, 3;...
  1, 3, 0.1, 1];  

% "Usage: output = multichannel_convolver( input, numberOfOutputs, blockLength, filterLength, maxRoutings, maxFilters. routings, filters )";
output = multichannel_convolver( input, numberOfOutputs, bs, filterLength, maxRoutings, maxFilters, routings, filters.' );