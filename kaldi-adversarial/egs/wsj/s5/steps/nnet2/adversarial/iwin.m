function x = iwin(X,cf)

N =    cf.nfft;     % FFT-L�nge
O =    cf.overlap;  % �berlappung
R =     N-O;        % Rahmenvorschub

L = size(X,2);

x = zeros((L-1)*R+N,1);
xmatrix = X;
for l = 0:L-1
    x(1+l*R:l*R+N) = x(1+l*R:l*R+N) + xmatrix(:,l+1);   
end

l = L*R+1;
x(1:O) = 2 * x(1:O);
x(l:end) = 2 * x(l:end);
