
// MFSK transmitter, Pawel Jalocha, December 2004

// ====================================================================

#include <stdio.h>
#include <ctype.h>

#include "mfsk.h"
#include "sound.h"
#include "stdinr.h"

// ====================================================================

int PlaybackFile      = 0;       // play back the file through the sound card
int LogText           = 0;       // log the decoded text to a file
int LogAudio          = 0;       // log the received audio to a file

SoundDevice Sound;            // sound card
size_t SampleRate=8000;       // sample rate we request from the sound card
char DeviceName[16] = "/dev/dsp";  // sound card device name
char *InputFileName=0;        // the name of the input file (specified on the command line)
char *AudioFileName;          // file name to save audio

MFSK_Parameters<float> Parameters; // parameters to the receiver
MFSK_Transmitter<float> Transmitter;

FILE *InputFile;

/*
static int ReadKeyboard(void)
{ size_t Count=0;
  for( ; ; )
  { int Char=Stdin_Read();
    if(Char<0) return -1;
    if(Char==0) break;
    uint8_t Char8=(uint8_t)Char;
    Transmitter.PutChar(Char8);
    putchar(Char);
    Count++; }
  fflush(stdout);
  return Count; }
*/

int main(int argc, char *argv[])
{ 
  int Error;

  // read the command line options and the input file name (if specified)
  int arg;
  int Help=0;
  for(arg=1; arg<argc; arg++)
  { if(argv[arg][0]=='-') // if '-' then this is an option, otherwise the name of a file
    { int Error=Parameters.ReadOption(argv[arg]);
      if(Error<0)
	  { printf("Invalid parameter(s) in %s\n",argv[arg]); }
	  else if(Error==0)
	  { switch(argv[arg][1])
        { case 'd':
            if(isdigit(argv[arg][2]))
		    { strcpy(DeviceName,"/dev/dsp"); strcat(DeviceName,argv[arg]+2); }
            else if(argv[arg][2]=='/')
            { strcpy(DeviceName,argv[arg]+2); }
            else if(argv[arg][2]=='\0')
            { strcpy(DeviceName,"/dev/dsp"); }
            else
            { printf("Unreadable device number or name: %s\n",argv[arg]); Help=1; }
            break;
          default:
            Help=1;
            break;
        }
      }
    }
    else
    { if(InputFileName==0) InputFileName=argv[arg];
      else if(AudioFileName==0) AudioFileName=argv[arg];
	  else Help=1;
    }
  }

  if(Help)
  { printf("\n\
mfsk_tx [options] [<text file>] [<audio file>]\n\
 options:\n\
  -d<device>            the soundcard device number or name [/dev/dsp]\n\
"         );
    printf("%s\n",Parameters.OptionHelp());
    return -1; }

  if(InputFileName)
  { InputFile=fopen(InputFileName,"rt");
    if(InputFile==0)
    { printf("Can not open %s for read !\n",InputFileName);
      return -1; }
  }

  Stdin_SetRaw();

  Error=Parameters.Preset();
  if(Error<0)
  { printf("Parameters.Preset() => %d\n",Error); return -1; }

  // preset the transmitter's internal arrays (negative return means fatal error)
  Error=Transmitter.Preset(&Parameters);
  if(Error<0)
  { printf("Transmitter.Preset() => %d\n",Error); return -1; }

  if(Sound.OpenForWrite(DeviceName,SampleRate,AudioFileName)<0)
  { printf("Can not open the sound device or file ?\n");
    return -1; }

  int16_t AudioBuffer[Transmitter.MaxOutputLen];

  printf("MFSK transmitter by Pawel Jalocha, March 2006\n");
  Parameters.Print();

  if(InputFileName)
    printf("MFSK transmitting from %s ... press ENTER to stop\n",InputFileName);
  else
    printf("MFSK transmitting ... type text, press Ctrl-R to stop\n");

  Transmitter.Start();
  int Stop=0;
  for( ; ; )
  { if(Stop) break;
    if(!InputFileName)
    { int Key=Stdin_Read();
	  if(Key<0) Stop=1;
      else if(Key>0)
	  { if(Key==('R'-'A'+1)) Stop=1;
	    else if(InputFileName==0) Transmitter.PutChar((uint8_t)Key); }
    } else
    { int Char=fgetc(InputFile);
      if(Char==EOF) Stop=1;
      Transmitter.PutChar((uint8_t)Char);
    }

    uint8_t Char;
    while(Transmitter.GetChar(Char)>0)       // monitor the characters being transmitted
      printf("%c",Char);                      // and write them to the screen
    fflush(stdout);
    int Len=Transmitter.Output(AudioBuffer);     // read audio from the transmitter
    if(Sound.Write(AudioBuffer,Len)<Len) Stop=1; // write the audio into the soundcard

  }

  printf("\n");

  Stdin_Restore();

  Transmitter.Stop();

  for( ; ; )
  { if(!Transmitter.Running()) break;
    int Len=Transmitter.Output(AudioBuffer);
    Sound.Write(AudioBuffer,Len); }

  Sound.Close();

  if(InputFile) fclose(InputFile);

  return 0; }

