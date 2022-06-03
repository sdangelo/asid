x=linspace(1/256,1,256);
y=linspace(0,1,16);
r=round(127*sin(2*pi*x')*y);
f=fopen("lfoscaling.dat","w");
fwrite(f,r(:),"schar");
fclose(f);
