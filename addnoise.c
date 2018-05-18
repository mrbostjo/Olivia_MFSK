
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double UniformNoise(void)
{ return ((double)rand()+1.0)/((double)RAND_MAX+1.0); }

void WhiteNoise(double *I, double *Q)
{ double Power,Phase;
  Power=sqrt(-2*log(UniformNoise()));
  Phase=2*M_PI*UniformNoise();
  (*I)=Power*cos(Phase);
  (*Q)=Power*sin(Phase);
}

FILE *InpFile=NULL;
FILE *OutFile=NULL;

#define BuffSize 8192
short InpBuff[BuffSize];

int main(int argc, char *argv[])
{ int len,i; long Total;
  float RMS,Scale;
  double I,Q;

  if(argc<4)
  { printf("Usage: addnoise <input.sw> <output.sw> <noise RMS> [<scale>]\n");
    return 1; }
  InpFile=fopen(argv[1],"rb");
  if(InpFile==NULL)
  { printf("Can't open %s for input\n",argv[1]); return 1; }
  OutFile=fopen(argv[2],"wb");
  if(OutFile==NULL)
  { printf("Can't open %s for output\n",argv[2]); return 1; }

  len=sscanf(argv[3],"%f",&RMS);
  if(len!=1)
  { printf("Invalid noise RMS: %s\n",argv[3]); return 1; }

  if(argc>4)
  { len=sscanf(argv[4],"%f",&Scale);
    if(len!=1)
    { printf("Invalid scale: %s\n",argv[4]); return 1; }
  } else Scale=1.0;
  
  RMS*=32768;
  int Limit=32768;
  for(Total=0; ; )
  { len=fread(InpBuff,2,BuffSize,InpFile);
    if(len<=0) break;
    for(i=0; i<len; i++)
    { WhiteNoise(&I,&Q);
      int Out=(int)floor(Scale*(InpBuff[i]+I*RMS)+0.5);
      if(Out>Limit) Out=Limit;
      else if(Out<(-Limit)) Out=(-Limit);
	  InpBuff[i]=(short)Out; }
    fwrite(InpBuff,2,len,OutFile); Total+=len;
  }
  printf("Done, %ld samples processed\n",Total);
  fclose(OutFile); fclose(InpFile);
  return 0;
}
