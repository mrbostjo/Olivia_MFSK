#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "sound.h"
#include "stdinr.h"

// =================================================================

void GetTime(struct timeval &Time)
{ static struct timezone Timezone = { 0, 0 } ;
  gettimeofday(&Time,&Timezone); }

// =================================================================

int main(int argc, char *argv[])
{

  int Transmit=0;
  size_t SampleRate = 8000;

  const size_t AudioLen=4096;
  int16_t Audio[AudioLen];

  SoundDevice Sound;
  char *DeviceName="/dev/dsp";

  struct timeval Start,Stop;

  if(argc>1)
  { if(sscanf(argv[1],"%d",&SampleRate)!=1)
    { printf("Usage: rate_check [<rate>] [Transmit]\n"); return -1; }
  }

  if((argc>2)&&(tolower(argv[2][0])=='t')) Transmit=1;

  if(Transmit)
  { size_t Idx;
    for(Idx=0; Idx<AudioLen; Idx++)
	  Audio[Idx]=0;
  }

  int Error;
  if(Transmit) Error=Sound.OpenForWrite(DeviceName,SampleRate);
          else Error=Sound.OpenForRead(DeviceName,SampleRate);
  if(Error<0)
  { printf("Cannot open the soundcard: invalid sampling rate or another problem\n");
    return -1; }

  printf("\n\
Soundcard sampling rate measurement, (c) Pawel Jalocha, September 2005\n\
\n\
This is a measurement of the sampling rate of your soundcard taking\n\
the real time clock (RTC) as the reference. You need to leave this test\n\
running for some time (5-10 min). You will see that with time the measured\n\
sampling rate will stabilize. The accuracy of this measurement\n\
is limited by the (in)accuracy of the RTC but it should be enough\n\
to notice major (and unexpected) differencies in the actuall sampling rates.\n\
\n\
This measurement is for %s and the card is set for %d samples/sec :\n\n\
", Transmit ? "TRANSMIT":"RECEIVE", SampleRate);

  int Len;

  size_t Idx;
  for(Idx=0; Idx<16; Idx++)
  { if(Transmit) Len=Sound.Write(Audio,AudioLen);
            else Len=Sound.Read(Audio,AudioLen);
  }

  GetTime(Start);
  long Samples=0;

  for( ; ; )
  {
    if(Transmit) Len=Sound.Write(Audio,AudioLen);
            else Len=Sound.Read(Audio,AudioLen);
    if(Len<0) break;
    GetTime(Stop);
    Samples+=Len;
    GetTime(Stop);
    double TimeDiff=Stop.tv_sec-Start.tv_sec;
	TimeDiff+=(Stop.tv_usec-Start.tv_usec)/1000000.0;
    printf("%10ld samples/%10.3f sec = %10.3f samples/sec\r",
	        Samples, TimeDiff, Samples/TimeDiff);
    fflush(stdout);
    if(Stdin_Ready()>0) { getchar(); break; }
  }

  printf("\n");
  Sound.Close();

  return 0; }

