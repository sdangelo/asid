y=linspace(0,120,256);
r=round(y);
f=fopen("barmapping.dat","w");
fwrite(f,r(:),"uchar");
fclose(f);
