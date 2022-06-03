%
% getIR.m - extracts the IR from a logsweep output (input 20 to 20kHz, 10 s
% long, 2 s silence before and after)
%
% Copyright (C) Orastron srl unipersonale 2022
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, version 3 of the License.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with A-SID.  If not, see <http://www.gnu.org/licenses/>.
%
% File author: Stefano D'Angelo
%
% Based on the theory in
%
% A.Farina, "Simultaneous measurement of impulse response and distortion with
% a swept-sine technique", 108th AES Convention, Paris, France, February 2000.

% y: logsweep output
% fs: sample rate (Hz)
% h: IR
function h = getIR(y, fs)
  
fstart=20;
fend=20e3;
silence=2;
T=10;

% sweep generation
t=[0:round(T*fs)-1];
t1=round(fs*silence);
t2=round(T*fs+silence*fs);
t3=round(T*fs+2*silence*fs);
w1 = 2*pi*fstart/fs;
w2 = 2*pi*fend/fs;
K = T*fs*w1/log(w2/w1);
L = T*fs/log(w2/w1);
sweep(1:t1-1)=0;
sweep(t1:t2-1)=sin(K.*(exp(t./L)-1));
sweep(t2:t3-1)=0;
  
% inverse filter
w = w1*exp(t./(T*fs).*log(w2/w1));
env = 10.^(-6/20.*log2(w./w1));
sweep_inv(1:t1-1)=0;
sweep_inv(t1:t2-1)=fliplr(sweep(t1:t2-1)).*env;
sweep_inv(t2:t3-1)=0;

% deconvolution
h=fftconv(sweep_inv,y);
  
% extract IR
[a,i]=max(abs(h));
i -= round(fs/2);
h=h(i:i+fs-1);
  
endfunction
