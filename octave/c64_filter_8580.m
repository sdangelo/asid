%
% c64_filter_8580.m - MOS 8580 filter simulation
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

% x: input
% fs: sample rate (Hz)
% bypass: bypass mix in [0, 1]
% hp: hp mix in [0, 1]
% bp: bp mix in [0, 1]
% lp: lp mix in [0, 1]
% cutoff: value in [0, 2047]
% resonance: value in [0, 15]
% volume: value in [0, 1]
% y: output
function y = c64_filter_8580(x, fs, bypass, hp, bp, lp, cutoff, resonance, volume)
  
  % absolute constants
  
  kin = 5.838280339378168e-1;
  Vmin = -4.757;
  Vmax = 4.243;
  Ve_k = 0.026 * omega(159.6931258945051);
  
  % sample-rate dependent
  
  in_B0 = fs / (fs + 13.55344121872543);
  in_mA1 = (fs - 13.55344121872543) / (fs + 13.55344121872543);
  
  out_B0 = tan(50e3 / fs) / (1 + tan(50e3 / fs));
  out_mA1 = (1 - tan(50e3 / fs)) / (1 + tan(50e3 / fs));
  
  dc_B0 = fs / (fs + 3.141592653589793);
  dc_mA1 = (fs - 3.141592653589793) / (fs + 3.141592653589793);
  
  B0_low = fs + fs;
  k1_low = 1 / B0_low;
  pi_fs = 3.141592653589793 / fs;
  
  % control rate
  
  % cutoff
  
  freq = 6.430966835743548 * cutoff;
  if (freq >= 1)
    B0 = (6.283185307179586 * freq) / tan(pi_fs * freq);
    k1 = 1 / B0;
  else
    B0 = B0_low;
    k1 = k1_low;
  endif
  k2 = 6.283185307179586 * freq;
  
  % resonance
  
  k = -1.254295325783559 + resonance * (9.443329173578159e-2 + resonance * -2.369572413355313e-3);
  
  % cutoff or resonance
  
  Vhp_x1 = k1 * (k1 * k2 - k);
  Vhp_den = 1 / (k2 * Vhp_x1 + 1);
  
  Vhp_dVbp_xxz1 = Vhp_den * Vhp_x1;
  Vhp_dVlp_xxz1 = Vhp_den * -k1;
  Vhp_dVbypass = Vhp_den * -kin;
  
  % volume
  
  kvol = -1.0435 * volume;
  
  % initial states
  
  in_z1 = 0;
  Vbp_z1 = 0;
  dVbp_z1 = 0;
  Vlp_z1 = 0;
  dVlp_z1 = 0;
  out_z1 = 0;
  dc_z1 = 0;
  
  % audio rate
  
  y = 0 * x;
  for i=1:length(y)
    Vin = x(i);
    
    % input
    
    in_x1 = in_B0 * Vin;
    Vbypass = in_x1 + in_z1;
    in_z1 = in_mA1 * Vbypass - in_x1;
    
    % filter
    
    dVbp_xxz1 = B0 * Vbp_z1 + dVbp_z1;
    dVlp_xxz1 = B0 * Vlp_z1 + dVlp_z1;
    Vhp = Vhp_dVbp_xxz1 * dVbp_xxz1 + Vhp_dVlp_xxz1 * dVlp_xxz1 + Vhp_dVbypass * Vbypass;
    Vbp = k1 * (dVbp_xxz1 - k2 * Vhp);
    Vlp = k1 * (dVlp_xxz1 - k2 * Vbp);
    dVbp = B0 * Vbp - dVbp_xxz1;
    dVlp = B0 * Vlp - dVlp_xxz1;
    
    Vbp_z1 = Vbp;
    dVbp_z1 = dVbp;
    Vlp_z1 = Vlp;
    dVlp_z1 = dVlp;
    
    % mix
    
    Vmix = min(max(-1.59074074074074 * (hp * Vhp + bp * Vbp + lp * Vlp) + -0.8653168127329506 * bypass * Vbypass, Vmin), Vmax);
    
    % volume
    
    Vvol = min(max(kvol * Vmix, Vmin), Vmax);
    
    % out lowpass
    
    out_x1 = out_B0 * Vvol;
    Vb = out_x1 + out_z1;
    out_z1 = out_x1 + out_mA1 * Vb;
    
    % out buffer
    
    Ve = 0.026 * omega(38.46153846153846 * Vb + 159.6931258945051) - Ve_k;
    
    % dc block
    
    dc_x1 = dc_B0 * Ve;
    Vout = dc_x1 + dc_z1;
    dc_z1 = dc_mA1 * Ve - dc_x1;
    
    y(i) = Vout;
  endfor
  
endfunction

function y = omega(x)
  if(x <= 400)
    y = lambertw(exp(x));
  else
    y = x - log(x);
  endif
endfunction
