function [X, Delta]  = FFT_Analysis_withMax(Input, n, max_val)
%X = FFT_Analysis(Input, n)
%
%   Compute the auditory spectrum using the Fast Fourier Transform.
%   The spectrum X is expressed in dB. The size of the transform si 512 and
%   is centered on the 384 samples (12 samples per subband) used for the 
%   subband analysis. The first of the 384 samples is indexed by n:
%   ................................................
%      |       |  384 samples    |        |
%      n-64    n                 n+383    n+447
%
%   A Hanning window applied before computing the FFT.
%
%   Finaly, a normalisation  of the sound pressure level is done such that
%   the maximum value of the spectrum is 96dB; the number of dB added is 
%   stored in Delta output.
%
%   One should take care that the Input is not zero at all samples.
%   Otherwise W will be -INF for all samples.
%
%   See also 
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: FFT_Analysis.m,v 1.4 1998-07-08 11:29:26+01 fapp2 Exp $

%   References:
%    [1] Information technology -- Coding of moving pictures and associated
%        audio for digital storage media at up to 1,5 Mbits/s -- Part3: audio.
%        British standard. BSI, London. October 1993. Implementation of ISO/IEC
%        11172-3:1993. BSI, London. First edition 1993-08-01.
%
%   Legal notice:
%    This computer program is based on ISO/IEC 11172-3:1993, Information
%    technology -- Coding of moving pictures and associated audio for digital
%    storage media at up to about 1,5 Mbit/s -- Part 3: Audio, with the
%    permission of ISO. Copies of this standards can be purchased from the
%    British Standards Institution, 389 Chiswick High Road, GB-London W4 4AL, 
%    Telephone:+ 44 181 996 90 00, Telefax:+ 44 181 996 74 00 or from ISO,
%    postal box 56, CH-1211 Geneva 20, Telephone +41 22 749 0111, Telefax
%    +4122 734 1079. Copyright remains with ISO.
%-------------------------------------------------------------------------------
Common;

Delta = [];
X = [];

N = length(Input);

% Check input parameters
if (n + FFT_SHIFT - FFT_OVERLAP > N | n < 1)
   error('Input too short: not enough samples to compute the FFT.');
end

% Prepare the samples used for the FFT
% Add zero padding if samples are missing
s = Input(max(1, n - FFT_OVERLAP):min(N, n + FFT_SIZE - FFT_OVERLAP - 1)); 
s = s(:);

if (n - FFT_OVERLAP < 1)
   s = [zeros(FFT_OVERLAP - n + 1, 1); s];
end
if (N < n - FFT_OVERLAP + FFT_SIZE - 1)
   s = [s; zeros(n - FFT_OVERLAP + FFT_SIZE - 1 - N, 1)];
end

% Prepare the Hanning window
h = sqrt(8/3) * hanning(FFT_SIZE);

% Power density spectrum
X = max(20 * log10(abs(fft(s .* h)) / FFT_SIZE), MIN_POWER);

% Normalization to the reference sound pressure level of 96 dB
Delta = 96 - max_val;
X = X + Delta;