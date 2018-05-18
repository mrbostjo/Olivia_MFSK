
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

FILE *InpFile=NULL;
FILE *OutFile=NULL;

#define BuffSize 8192
short InpBuff[BuffSize];

int main(int argc, char *argv[])
{ int len,i; long Total; long NewSample;
  float Freq,Ampl; double Phase; float I;

  if(argc<5)
  { printf("Usage: addcarr <input.sw> <output.sw> <frequency> <amplitude>\n");
    return 1; }
  InpFile=fopen(argv[1],"rb");
  if(InpFile==NULL)
  { printf("Can't open %s for input\n",argv[1]); return 1; }
  OutFile=fopen(argv[2],"wb");
  if(OutFile==NULL)
  { printf("Can't open %s for input\n",argv[2]); return 1; }
  len=sscanf(argv[3],"%f",&Freq);
  if(len!=1)
  { printf("Invalid floating point number: %s\n",argv[3]); return 1; }
  len=sscanf(argv[4],"%f",&Ampl);
  if(len!=1)
  { printf("Invalid floating point number: %s\n",argv[4]); return 1; }

  Phase=0.0; Freq*=2*M_PI;
  for(Total=0; ; )
  { len=fread(InpBuff,2,BuffSize,InpFile);
    if(len<=0) break;
    for(i=0; i<len; i++)
    { I=cos(Phase); Phase+=Freq; if(Phase>=2*M_PI) Phase-=2*M_PI;
      NewSample=(long)floor(I*Ampl*32768.0+0.5);
      NewSample+=InpBuff[i];
      if(NewSample>32767L) NewSample=32767L;
	else if(NewSample<(-32768L)) NewSample=(-32768L);
      InpBuff[i]=(short)NewSample; }
    fwrite(InpBuff,2,len,OutFile); Total+=len;
  }
  printf("Done, %ld samples processed\n",Total);
  fclose(OutFile); fclose(InpFile);
  return 0;
}
