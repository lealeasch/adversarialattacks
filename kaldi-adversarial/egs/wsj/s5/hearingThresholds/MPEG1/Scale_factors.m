function scf = Scale_factors(S)
%scf = Scale_factors(S)
%   For each subband S(i, :), the maximum of the absolute value of 12
%   samples S(i, 1) ... S(i, 12) is determined. The next largest value of
%   the scale factor in the scale factor table [1, pp. 45] and [1, pp 70]
%   is chosen.
%
%   See also 
   
%   Author: Fabien A.P. Petitcolas
%           Computer Laboratory
%           University of Cambridge
%   Copyright (c) 1998 by Fabien A.P. Petitcolas
%   $Id: Scale_factors.m,v 1.3 1998-06-24 10:20:22+01 fapp2 Exp $

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

Table_scf = [
   2.00000000000000; 1.58740105196820; 1.25992104989487; 1.00000000000000; 
   0.79370052598410; 0.62996052494744; 0.50000000000000; 0.39685026299205; 
   0.31498026247372; 0.25000000000000; 0.19842513149602; 0.15749013123686; 
   0.12500000000000; 0.09921256574801; 0.07874506561843; 0.06250000000000; 
   0.04960628287401; 0.03937253280921; 0.03125000000000; 0.02480314143700; 
   0.01968626640461; 0.01562500000000; 0.01240157071850; 0.00984313320230; 
   0.00781250000000; 0.00620078535925; 0.00492156660115; 0.00390625000000; 
   0.00310039267963; 0.00246078330058; 0.00195312500000; 0.00155019633981; 
   0.00123039165029; 0.00097656250000; 0.00077509816991; 0.00061519582514; 
   0.00048828125000; 0.00038754908495; 0.00030759791257; 0.00024414062500; 
   0.00019377454248; 0.00015379895629; 0.00012207031250; 0.00009688727124; 
   0.00007689947814; 0.00006103515625; 0.00004844363562; 0.00003844973907; 
   0.00003051757813; 0.00002422181781; 0.00001922486954; 0.00001525878906; 
   0.00001211090890; 0.00000961243477; 0.00000762939453; 0.00000605545445; 
   0.00000480621738; 0.00000381469727; 0.00000302772723; 0.00000240310869;
   0.00000190734863; 0.00000151386361; 0.00000120155435; 1E-20
];

N = length(Table_scf);

for i = 1:32,
   si_min = max(abs(S(:, i)));
   if (si_min > Table_scf(1))
      % This usually happen when the input signal has values
      % greater than one.
      disp('Warning: cannot find scale factor.');
      scf(i) = Table_scf(1);
   else
      j = 0;
      while (j < N & si_min > Table_scf(N - j))
         j = j + 1;
      end
      scf(i) = Table_scf(N - j);
   end
end
