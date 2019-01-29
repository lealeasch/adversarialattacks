function  [LTt, LTn] = Individual_masking_thresholds(X, Tonal_list, ...
   Non_tonal_list, TH, Map)
%[LTt, LTn] = Individual_masking_thresholds(X, Tonal_list, ...
%   Non_tonal_list, TH, Map)
%
%   Compute the masking effect of both tonal and non_tonal components on
%   the neighbouring spectral frequencies [1, pp. 113]. The strength os the
%   masker is summed with the masking index and the masking function.
%
%   See also 
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: Individual_masking_thresholds.m,v 1.3 1998-06-24 13:34:17+01 fapp2 Exp $

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

% Individual masking thresholds for both tonal and non-tonal 
% components are set to -infinity since the masking function has
% infinite attenuation beyond -3 and +8 barks, that is the component
% has no masking effect on frequencies beyond thos ranges [1, pp. 113--114]
if isempty(Tonal_list)
   LTt = [];
else
   LTt = zeros(length(Tonal_list(:, 1)), length(TH(:, 1))) + MIN_POWER;
end
LTn = zeros(length(Non_tonal_list(:, 1)), length(TH(:, 1))) + MIN_POWER;

% Only a subset of the samples are considered for the calculation of
% the global masking threshold. The number of these samples depends
% on the sampling rate and the encoding layer. All the information
% needed is in TH which contains the frequencies, critical band rates
% and absolute threshold.
for i = 1:length(TH(:, 1))
   zi = TH(i, BARK);  % Critical band rate of the frequency considered   
   
   if not(isempty(Tonal_list))
	   % For each tonal component
	   for k = 1:length(Tonal_list(:, 1)),
	      j  = Tonal_list(k, INDEX);
	      zj = TH(Map(j), BARK); % Critical band rate of the masker
	      dz = zi - zj;          % Distance in Bark to the masker
	      
	      if (dz >= -3 & dz < 8)
	         
	         % Masking index
	         avtm = -1.525 - 0.275 * zj - 4.5;
	         
	         % Masking function
	         if (-3 <= dz & dz < -1)
	            vf = 17 * (dz + 1) - (0.4 * X(j) + 6);
	         elseif (-1 <= dz & dz < 0)
	            vf = (0.4 * X(j) + 6) * dz;
	         elseif (0 <= dz & dz < 1)
	            vf = -17 * dz;
	         elseif (1 <= dz & dz < 8)
	            vf = - (dz - 1) * (17 - 0.15 * X(j)) - 17;
	         end
         
	         LTt(k, i) = X(j) + avtm + vf;
	      end
      end
   end
   
   
   % For each non-tonal component
   for k = 1:length(Non_tonal_list(:, 1)),
      j  = Non_tonal_list(k, INDEX);
      zj = TH(Map(j), BARK); % Critical band rate of the masker
      dz = zi - zj;          % Distance in Bark to the masker
      
      if (dz >= -3 & dz < 8)
         
         % Masking index
         avnm = -1.525 - 0.175 * zj - 0.5;
         
         % Masking function
         if (-3 <= dz & dz < -1)
            vf = 17 * (dz + 1) - (0.4 * X(j) + 6);
         elseif (-1 <= dz & dz < 0)
            vf = (0.4 * X(j) + 6) * dz;
         elseif (0 <= dz & dz < 1)
            vf = -17 * dz;
         elseif (1 <= dz & dz < 8)
            vf = - (dz - 1) * (17 - 0.15 * X(j)) - 17;
         end
         
         LTn(k, i) = X(j) + avnm + vf;
      end
   end
end

% Add the indicudual masking thresholds to the existing graph
if (DRAW)
   if not(isempty(Tonal_list))
		hold on;
		for j = 1:length(Tonal_list(:, 1))
	   	plot(TH(:, INDEX), LTt(j, :), 'r:');
	   end
	   disp('Masking threshold for tonal components.');
      pause;
   end
	for j = 1:length(Non_tonal_list(:, 1))
	   plot(TH(:, INDEX), LTn(j, :), 'g:');
	end
   hold off;
   disp('Masking threshold for non-tonal components.');
   pause;
end