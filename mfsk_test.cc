
// ===================================================================

// A simulation for the MFSK_Transmitter, MFSK_Receiver
// and a (very) noisy path between the two.

// ===================================================================

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mfsk.h"
#include "noise.h"
#include "bitcount.h"

// ===================================================================

size_t CountDiffs(char *Input, char *Output, size_t Len)
{ size_t Idx;
  size_t Diffs=0;
  for(Idx=0; Idx<Len; Idx++)
  { if(Input[Idx]!=Output[Idx]) Diffs+=1; }
  return Diffs; }

double TotalSignalEnergy=0;
double TotalNoiseEnergy=0;

void AddNoise(float *Data, size_t Len, float RMS=1.0)
{ size_t Idx;
  Cmpx<float> Noise;
  for(Idx=0; Idx<Len; Idx++)
  { float Signal=Data[Idx];
    TotalSignalEnergy+=Signal*Signal;
    WhiteNoise(Noise,RMS);
	Data[Idx]+=Noise.Re;
    TotalNoiseEnergy+=Noise.Re*Noise.Re;
  }
}

// ===================================================================

MFSK_Parameters<float>  Parameters;

MFSK_Transmitter<float> Transmitter;
MFSK_Receiver<float>    Receiver;

int main(int argc, char *argv[])
{ int Error;

  time_t Now;
  time(&Now);
  srand(Now);
/*
  Parameters.InputSampleRate=8000.0;
  Parameters.BitsPerSymbol=5;
  Parameters.RxSyncMargin=2;
  Parameters.RxSyncIntegLen=8;
*/

  Parameters.ReadOption("-T32");
  Parameters.ReadOption("-B1000");
  Parameters.ReadOption("-R8000.0/8000.0");
  Parameters.ReadOption("-M2");
  Parameters.ReadOption("-I8");

  Error=Parameters.Preset();
  if(Error<0) { printf("Parameters.Preset() => %d\n",Error); return -1; }
  Parameters.Print();

  Error=Transmitter.Preset(&Parameters);
  if(Error<0) { printf("Transmitter.Preset() => %d\n",Error); return -1; }

  Error=Receiver.Preset(&Parameters);
  if(Error<0) { printf("Receiver.Preset() => %d\n",Error); return -1; }

  size_t MessageLen=128;
  char InputMessage[MessageLen];

  float NoiseRMS=3.6; // 3.0 => -16dB/4kHz, 3.7 => -18dB/4kHz

  size_t Idx;

  for(Idx=0; Idx<MessageLen; Idx++)
    InputMessage[Idx]=Idx; // rand()&0x7F;

  for(Idx=0; Idx<40; Idx++)
  { Transmitter.PutChar(0); }

  for(Idx=0; Idx<MessageLen; Idx++)
  { uint8_t Char=InputMessage[Idx];
    Transmitter.PutChar(Char); }

  Transmitter.Start();

  for(Idx=0; Idx<((MessageLen/5+10+10)*64); Idx++)
  {

    float *OutputPtr;
    int Len=Transmitter.Output(OutputPtr);

	AddNoise(OutputPtr,Len,NoiseRMS);

    Receiver.Process(OutputPtr,Len);

    if((Idx&0x1F)==0)
     printf("SyncSNR=%4.1f, %+4.2f Hz, %+5.1f Hz/min, %4.0f ppm, %+4.1f dB\n",
	        Receiver.SyncSNR(), Receiver.FrequencyOffset(),
			60*Receiver.FrequencyDrift(), 1E6*Receiver.TimeDrift(),
			Receiver.InputSNRdB() );

  }

  Receiver.Flush();

  char OutputMessage[MessageLen+128+1];

  for(Idx=0; ; Idx++)
  { uint8_t Char;
    if(Receiver.GetChar(Char)==0) break;
    if(Idx>=(MessageLen+128)) break;
    OutputMessage[Idx]=Char;
  } OutputMessage[Idx]=0;
  size_t OutputMessageLen=Idx;

  printf("Receiver output [%d] : ", OutputMessageLen);
  for(Idx=0; Idx<OutputMessageLen; Idx++)
  { char Char=OutputMessage[Idx];
    if(Char>' ') printf("%c",Char);
            else printf("^%c",0x40+Char);
  } printf("\n");

  size_t Ofs;
  size_t MinOfs=0; size_t MinDiffs=MessageLen;
  for(Ofs=0; (Ofs+MessageLen)<OutputMessageLen; Ofs++)
  { size_t Diffs=CountDiffs(InputMessage,OutputMessage+Ofs,MessageLen);
    // printf(" %d",Diffs);
    if(Diffs<MinDiffs) { MinOfs=Ofs; MinDiffs=Diffs; }
  } // printf("\n");
  printf("Character errors: %d/%d (%d)\n",MinDiffs, MessageLen, MinOfs);

  double SignalToNoise=TotalSignalEnergy/TotalNoiseEnergy;
  printf("Signal/Noise = %5.3f = %+5.1f dB [4 kHz bandwidth]\n",
         SignalToNoise, 10*log(SignalToNoise)/log(10.0));

  return 0; }
