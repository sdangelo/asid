freq=400+(2200-400).*(linspace(0,1,256).^3);
cutoff=round(0.1554976142066175*freq);
low=bitand(cutoff,7);
high=bitshift(cutoff,-3);
f=fopen("cutoffmapping.dat","w");
fwrite(f,low(:),"uchar");
fwrite(f,high(:),"uchar");
fclose(f);
