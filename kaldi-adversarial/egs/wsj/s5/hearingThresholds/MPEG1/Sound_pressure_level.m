function Lsb = Sound_pressure_level(X, scf)
%X = Sound_pressure_level(X, scf)
%
%   The sound pressure level Lsb is computed for every subband.
%   X is the normalised power density spectrum and scf are the 32 scale
%   factors (one per band).
%
%   See also  Scale_factors
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: Sound_pressure_level.m,v 1.2 1998-06-22 17:47:56+01 fapp2 Exp $

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

% Check input parameters
if (length(X) ~= FFT_SIZE)
   error('Unexpected power density spectrum size.');
end

if (length(scf) ~= N_SUBBAND)
   error('Unexpected number of scalefactors');
end

% Pick the 32 sound pressure level of the spectral line with maximum amplitude
% in the frequency range corresponding to subband i and compute the
% sound pressure level Lsb.
Xmin = min(X);
n = FFT_SIZE / 2 / N_SUBBAND; % Size of each subband

for i = 1:N_SUBBAND,
   local_max = Xmin;
   for j = 1:n,
      local_max = max(X((i - 1) * n + j), local_max);
   end
   Lsb(i) = max(local_max, 20 * log10(scf(i) * 32768) - 10);
end
