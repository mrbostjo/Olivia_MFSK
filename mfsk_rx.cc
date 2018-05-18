
// MFSK receiver, Pawel Jalocha, December 2004

// ====================================================================

#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include "mfsk.h"
#include "sound.h"
#include "term.h"

// ====================================================================

// convert time into an ASCII form suitable for a file name
void AsciiTime(char *Ascii, time_t *Time)
{ static const char *MonthName[12] =
    { "jan", "feb", "mar", "apr", "may", "jun",
      "jul", "aug", "sep", "oct", "nov", "dec", } ;
  struct tm *TM = localtime(Time);
  sprintf(Ascii,"%02d%s%04d_%02d%02d%02d",
          TM->tm_mday, MonthName[TM->tm_mon], TM->tm_year+1900,
          TM->tm_hour, TM->tm_min, TM->tm_sec);
}

// ====================================================================

// program parameters (can be changed with the command line options)

int PlaybackFile      = 0;       // play back the file through the sound card
int LogText           = 0;       // log the decoded text to a file
int LogAudio          = 0;       // log the received audio to a file

SoundDevice Sound;            // sound card
size_t SampleRate=8000;       // sample rate we request from the sound card
char DeviceName[16] = "/dev/dsp";  // sound card device name
char *InputFileName=0;        // the name of the input file (specified on the command line)

char AudioFileName[32];       // file name to save audio
char TextFileName[32];        // file name to save decoded text

const size_t AudioBufferLen=2048;
int16_t AudioBuffer[AudioBufferLen]; // audio buffer

MFSK_Parameters<float> Parameters; // parameters to the receiver

MFSK_Receiver<float> Receiver;  // Receiver to demodulate and decode

SplitTerm Terminal;              // Terminal to print the decoded text and display parameters

int main(int argc, char *argv[])
{ int Error;

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
        { case 'p':
            PlaybackFile=1;
            break;
          case 'l':
            LogText=1;
            break;
          case 'L':
            LogAudio=1;
            break;
          case 'd':
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
	                  else Help=1;
    }
  }

  if(Help)
  { printf("\n\
mfsk_rx [options] [<audio file>]\n\
 options:\n\
  -d<device>            the soundcard device number or name [/dev/dsp]\n\
  -p                    playback the file through the soundcard\n\
  -l                    log the decoded text to a file\n\
  -L                    log the received audio to a file\n\
"         );
    printf("%s\n",Parameters.OptionHelp());
    return -1; }

  time_t Now;
  time(&Now);
  char TimeStr[32];
  AsciiTime(TimeStr,&Now);

  sprintf(TextFileName,"mfsk_%s.log",TimeStr);
  sprintf(AudioFileName,"mfsk_%s.sw",TimeStr);

  // open the sound card (or specified file) for read
  if(InputFileName) // if input file specified, read audio from this file
  { if(Sound.OpenFileForRead(InputFileName,SampleRate, PlaybackFile ? DeviceName:0)<0)
    { printf("Can not open the sound device or file ?\n");
      return -1; }
  } else            // if no input file specified, read audio from the sound card
  { if(Sound.OpenForRead(DeviceName,SampleRate,LogAudio ? AudioFileName:0)<0)
    { printf("Can not open the sound device or file ?\n");
      return -1; }
  }

  Error=Parameters.Preset();
  if(Error<0)
  { printf("Parameters.Preset() => %d\n",Error); return -1; }

  // preset the receiver's internal arrays (negative return means fatal error)
  Error=Receiver.Preset(&Parameters);
  if(Error<0)
  { printf("Receiver.Preset() => %d\n",Error); return -1; }

  Error=Terminal.Preset(0,LogText ? TextFileName:0);
  if(Error<0)
  { printf("Terminal.Preset() => %d\n",Error); return -1; }

  char Mode[80];
  sprintf(Mode,"Mode: %d tones, %d Hz, %4.2f baud, %3.1f sec/block, %3.1f chars/sec",
           Parameters.Carriers,
		   Parameters.Bandwidth,
		   Parameters.BaudRate(),
		   Parameters.BlockPeriod(),
		   Parameters.CharactersPerSecond() );

  Terminal.RxStatUpp(Mode);

  char Status[80];
  sprintf(Status,"Status:");

  Terminal.RxStatLow(Status);

  size_t SymbolCounter=0;
  for( ; ; SymbolCounter++)
  { int Len=Sound.Read(AudioBuffer,AudioBufferLen);
    if(Len<=0) break;  // if no more audio data or an error then break the loop

    // let the receiver process the audio in the buffer
    Receiver.Process(AudioBuffer,Len);

    // update the status: S/N and frequency offset
    sprintf(Status,"Rx S/N: %4.1f,  %+5.1f dB,   %+4.1f/%4.1f Hz,  %+5.1f Hz/min,  %+5.0f ppm",
	         Receiver.SyncSNR(), Receiver.InputSNRdB(),
			 Receiver.FrequencyOffset(), Parameters.TuneMargin(),
			 60*Receiver.FrequencyDrift(), 1E6*Receiver.TimeDrift()  );
    Terminal.RxStatLow(Status);

    // see, if there are any character coming out of the decoder ?
    for( ; ; )
    { uint8_t Char;
      if(Receiver.GetChar(Char)<=0) break; // break the loop if no more characters
      Terminal.RxChar_Filtered(Char); // otherwise print the character on the terminal
    }

    // if user pushed a key, then break the loop
    int Key;
    if(Terminal.UserInp(Key)) { break; }
  }

  // flush the receiver:
  Receiver.Flush();
  // and then check for decoded characters
  for( ; ; )
  { uint8_t Char;
    if(Receiver.GetChar(Char)<=0) break; // break the loop if no more characters
    Terminal.RxChar_Filtered(Char); // otherwise print the character on the terminal
  }

  sleep(3);          // wait three seconds before closing the terminal window
  Terminal.Close();  // close the terminal
  Sound.Close();     // close the audio device

  return 0; }
