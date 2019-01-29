function S = Analysis_subband_filter(Input, n, C)
%S = Analysis_subband_filter(Input, n, C)
%   Returns the 32 subband samples S(i) defined in [1, pp. 67,78]
%   n is the index in Input where the 32 `new'samples are located.
%   C is the analysis window defined in [1, pp.68--69].
%
%   See also Analysis_window
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: Analysis_subband_filter.m,v 1.3 1998-07-07 14:39:54+01 fapp2 Exp $

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

nmax = length(Input);

% Check input parameters
if (n + 31 > nmax | n < 1)
   error('Unexpected analysis index.');
end

% Build an input vector X of 512 elements. The most recent sample
% is at position 512 while the oldest element is at position 1.
% Padd with zeroes if the input signal does not exist.
%  ...........................................................
%         |         480 samples        |  32 samples   |
%         n-480                         n              n+31
X = Input(max(1, n - 480):n + 31); % / 32768
X = X(:);
X = [zeros(512 - length(X), 1); X];

% Window vector X by vector C. This produces the Z buffer.
Z = X .* C;

% Partial calculation: 64 Yi coefficients
Y = zeros(1, 64);
for i = 1 : 64,
   for j = 0 : 7,
      Y(i) = Y(i) + Z(i + 64 * j);
   end
end

% Calculate the analysis filter bank coefficients
for i = 0 : 31,
   for k = 0 : 63,
      M(i + 1, k + 1) = cos((2 * i + 1) * (k - 16) * pi / 64);
   end
end

% Calculate the 32 subband samples Si
S = zeros(1, 32);
for i = 1 : 32,
   for k = 1 : 64,
      S(i) = S(i) + M(i, k) * Y(k);
   end
end
