function [DFlags, DTonal_list, DNon_tonal_list] = ...
   Decimation(X, Tonal_list, Non_tonal_list, Flags, TH, Map)
%[DFlags, DTonal_list, DNon_tonal_list] = ...
%   Decimation(X, Tonal_list, Non_tonal_list, Flags, TH, Map)
%
%   Components which are below the auditory threshold or are less than one 
%   half of a critical band width from a neighbouring component are
%   eliminated [1, pp. 113]. DFlags, DTonal_list and DNon_tonal_list
%   contain the list of flags, tonal components and non-tonal components after
%   decimation.
%
%   See also Find_tonal_components
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: Decimation.m,v 1.3 1998-06-24 13:33:29+01 fapp2 Exp $

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

if (DRAW),
   t = 1:length(X);
end

DFlags = Flags; % Flags after decimation


% Tonal or non-tonal components are not considered if lower than
% the absolute threshold of hearing found in TH(:, ATH).

% Non tonal case
DNon_tonal_list = [];
if not(isempty(Non_tonal_list))
	for i = 1:length(Non_tonal_list(:, 1)),
	   k = Non_tonal_list(i, INDEX);
	   %if (k > length(Map))
	   %   DFlags(k) = IRRELEVANT;
	   %else
	   if (Non_tonal_list(i, SPL) < TH(Map(k), ATH))
	   	DFlags(k) = IRRELEVANT;
		else
		   DNon_tonal_list = [DNon_tonal_list; Non_tonal_list(i, :)];
	   end
	   %end
	end
	if (DRAW),
	   disp('Non-tonal components v. absolute threshold.');
	   plot(t, X(t), Non_tonal_list(:, INDEX), Non_tonal_list(:, SPL), 'go', ...
	      DNon_tonal_list(:, INDEX), DNon_tonal_list(:, SPL), 'yo', ...
	      TH(:, INDEX), TH(:, ATH));
	   xlabel('Frequency index'); ylabel('dB'); 
	   title('Non-tonal components v. absolute threshold.');
	   axis([0 256 -20 100]); pause;
   end
end

% Tonal case (first part)
DTonal_list = [];
if not(isempty(Tonal_list))
	for i = 1:length(Tonal_list(:, 1)),
	   k = Tonal_list(i, INDEX);
	   %if (k > length(Map))
	   %   DFlags(k) = IRRELEVANT;
	   %else
	   if (Tonal_list(i, SPL) < TH(Map(k), ATH))
		   DFlags(k) = IRRELEVANT;
	   else
		   DTonal_list = [DTonal_list; Tonal_list(i, :)];
	   end
	   %end
	end
	if (DRAW),
	   disp('Tonal components v. absolute threshold.');
	   plot(t, X(t), Tonal_list(:, INDEX), Tonal_list(:, SPL), 'ro', ...
	      DTonal_list(:, INDEX), DTonal_list(:, SPL), 'mo', TH(:, INDEX), ...
	      TH(:, ATH)); axis([0 256 0 100]);
	   xlabel('Frequency index'); ylabel('dB'); 
	   title('Tonal components v. absolute threshold.');
	   axis([0 256 -20 100]); pause;
   end
end

% Eliminate tonal components that are less than one half of
% critical band width from a neighbouring component.
% (second part of tonal case)
if not(isempty(DTonal_list))
	i = 1;
	while (i < length(DTonal_list(:, 1))),
	   k      = DTonal_list(i, INDEX);
	   k_next = DTonal_list(i + 1, INDEX);
		if (TH(Map(k_next), BARK) - TH(Map(k), BARK) < 0.5)
		   if (DTonal_list(i, SPL) < DTonal_list(i + 1, SPL))
	         DTonal_list = DTonal_list([1:i - 1, ...
	               i + 1:length(DTonal_list(:, 1))], :);
	         DFlags(k) = IRRELEVANT;
	      else
	         DTonal_list = DTonal_list([1:i, ...
	               i + 2:length(DTonal_list(:, 1))], :);
	         DFlags(k_next) = IRRELEVANT;
		   end
	   end
	   i = i + 1;
	end
	if (DRAW),
		disp('Tonal components too closed to each other eliminated.');
	   plot(t, X(t), Tonal_list(:, INDEX), Tonal_list(:, SPL), 'ro', ...
	      DNon_tonal_list(:, INDEX), DNon_tonal_list(:, SPL), 'go', ...
	      TH(:, INDEX), TH(:, ATH)); axis([0 256 0 100]);
	   xlabel('Frequency index'); ylabel('dB');
	   title('Tonal components too closed to each other eliminated.');
	   axis([0 256 -20 100]); pause;
   end
end

