%   This sets the `global' variables use in the other functions.
%   They correspond to MPEG audio layer I with psychoacoustic model 1
%   as described in [1].
%
%   See also 
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: Common.m,v 1.4 1998-07-07 14:39:54+01 fapp2 Exp $

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

FFT_SHIFT   = 384;
FFT_SIZE    = 512;
FFT_OVERLAP = (FFT_SIZE - FFT_SHIFT) / 2;

N_SUBBAND = 32;

% Flags for tonal analysis
NOT_EXAMINED = 0;
TONAL        = 1;
NON_TONAL    = 2;
IRRELEVANT   = 3;

MIN_POWER = -200;

% Set to one to see the graphs and explanation text
DRAW = 0;

% Indices used in tables like the threshold table
% or in the list of tonal and non-tonal components.
INDEX = 1;
BARK  = 2;
SPL   = 2;
ATH   = 3;
