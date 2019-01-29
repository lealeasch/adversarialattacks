function CB = Table_critical_band_boundaries(Layer, fs)
%CB = Table_crital_band_boundaries(Layer, fs)
%
%   Return the index in the absolute threshold table for the critical band 
%   boundaries definied in [1, pp. 123] for Layer at sampling rate fs (Hz).
%
%   See also Table_absolute_threshold
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   $Id: Table_critical_band_boundaries.m,v 1.2 1998-06-22 17:47:56+01 fapp2 Exp $

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

if (Layer == 1)
   if (fs == 44100)
      CB = [1, 2, 3, 5, 6, 8, 9, 11, 13, 15, 17, 20, 23, 27, 32, 37, 45, 50, 55, 61, 68, 75, 81, 93, 106]';
    else
       error('Frequency not supported.');
    end
 else
    error('Layer not supported.');
 end
 