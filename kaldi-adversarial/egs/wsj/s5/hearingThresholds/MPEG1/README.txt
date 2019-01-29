MPEG for MATLAB
---------------

This is an implementation of the MPEG psychoacoustic model 1, layer I,
for MATLAB. It has been tested on MATLAB 5.2 running on Windows NT.
All you have to do is to copy the ".m" files into a directory and call
the "Test_MPEG" rountine from MATLAB. Feel free to modify this routine
in order to use your own sample signal.

The "Common.m" file contains some global variables used by other
functions. In particular you can set or reset the "DRAW" variable
which tells whether graphs should be displayed at each steps of the
process; this is very useful if you want to _see_ how MPEG works.

This implementation is based on

    Information technology -- Coding of moving pictures and associated
    audio for digital storage media at up to about 1,5 Mbit/s -- Part
    3: audio. British Standard, BSI, London. First edition 1993-08-01.
    October 1993. Implementation of ISO/IEC 11172-3:1993. ISBN
    0-580-22594-1

This implementation is freely available from

    <http://www.cl.cam.ac.uk/~fapp2/software/mpeg/>


Acknowledgement:
The author of is a Ph.D student supported by the Intel Corporation 
under the grant `robustness of information hiding.'


Legal notice:
This computer program is based on ISO/IEC 11172-3:1993, Information
technology -- Coding of moving pictures and associated audio for digital
storage media at up to about 1,5 Mbit/s -- Part 3: Audio, with the
permission of ISO. Copies of this standards can be purchased from the
British Standards Institution, 389 Chiswick High Road, GB-London W4 4AL, 
Telephone:+ 44 181 996 90 00, Telefax:+ 44 181 996 74 00 or from ISO,
postal box 56, CH-1211 Geneva 20, Telephone +41 22 749 0111, Telefax
+4122 734 1079. Copyright remains with ISO.


Fabien.

--
Fabien A.P. Petitcolas, University of Cambridge, Computer Laboratory
fapp2@cl.cam.ac.uk, <http://www.cl.cam.ac.uk/~fapp2/>