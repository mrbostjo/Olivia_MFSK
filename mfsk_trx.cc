
// MFSK transmitter and receiver, Pawel Jalocha, December 2004

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

size_t SampleRate      = 8000;    // sample rate we request from the sound card       [Hz]
char DeviceName[16]    = "/dev/dsp";  // sound card device name
int LogText            = 0;       // log the decoded text to a file
int LogAudio           = 0;       // log the received audio to a file

MFSK_Parameters<float> Parameters; // parameters to the receiver

SoundDevice Sound;               // sound card

size_t TxBufferLen=0;
int16_t *TxBuffer;                     // transmitter audio buffer

MFSK_Transmitter<float> Transmitter;  // Transmitter

const size_t RxBufferLen=1024;
int16_t RxBuffer[RxBufferLen];        // receiver audio buffer (fixed size)

MFSK_Receiver<float> Receiver;        // Receiver

SplitTerm Terminal;                    // Terminal

char TextFileName[32];

// ================================================================================

int Transmit=0;    // transmit or receive mode

int ExitReq=0;     // request exit
int TransmitReq=0; // request to switch to transmit mode
int ReceiveReq=0;  // request to swtich to receive mode

// read the keyboard
static int ReadKeyboard(void)
{ int Key;
  int Error=Terminal.UserInp(Key);     // read the next key
  if(Error<=0) return Error;           // if no key or error: return zero or error
  if((Key>=' ')||(Key=='\b'))          // if a visible character or a tab
  { Error=Transmitter.PutChar(Key);     // put the character into the Transmitter queue
    if(Error>0) Terminal.TxChar(Key); } // if queue not full: print the character in the Tx window
  else if(Key=='\r')                    // if carrier return (ENTER)
  { Error=Transmitter.PutChar('\r');     // put CR into the transmitter queue
    if(Error>0) Terminal.TxChar('\n'); } // and print in the Tx window
  else if(Key==('X'-'@')) ExitReq=1;     // Ctrl-X => set the exit request
  else if(Key==('R'-'@')) ReceiveReq=1;  // Ctrl-R => set the receive request
  else if(Key==('T'-'@')) TransmitReq=1; // Ctrl-T => set the transmitt request
  return 1; }                            // return 1 (one character was read)

// read the decoded characters from the receiver and print them in the Rx window
static int ReadReceiverOutput(void)
{ uint8_t Char;
  if(Receiver.GetChar(Char)<=0) return 0; // return if no more characters
  Terminal.RxChar_Filtered(Char); // otherwise print the character on the terminal
  return 1; }

// read audio from the sound card and feed it into the Receiver
static int FeedReceiver(void)
{ int Len=Sound.Read(RxBuffer,RxBufferLen); // read the audio from the sound card
  if(Len<0) return -1;
  Receiver.Process(RxBuffer,Len);           // process the audio through the receiver
  return Len; }

static void FlushReceiver(void)
{ Receiver.Flush();
  for( ; ReadReceiverOutput()>0; ) ; }

// print the mode details: number of tones, bandwidth, baud rate, character rate
static void PrintReceiverMode(void)
{ char Mode[80];
  sprintf(Mode,"Mode: %d tones, %d Hz, %4.2f baud, %3.1f sec/block, %3.1f chars/sec",
           Parameters.Carriers,
		   Parameters.Bandwidth,
		   Parameters.BaudRate(),
		   Parameters.BlockPeriod(),
		   Parameters.CharactersPerSecond() );
  Terminal.RxStatUpp(Mode); }

// update the receiver status: signal-to-noise and frequency offset
static void PrintReceiverStatus(void)
{ static char Status[80];
  sprintf(Status,"Rx S/N: %4.1f,  %+5.1f dB,   %+4.1f/%4.1f Hz,  %+5.1f Hz/min,  %+5.0f ppm",
	       Receiver.SyncSNR(), Receiver.InputSNRdB(),
		   Receiver.FrequencyOffset(), Parameters.TuneMargin(),
		   60*Receiver.FrequencyDrift(), 1E6*Receiver.TimeDrift()  );
  Terminal.RxStatLow(Status); }

// switch to receive mode
static int SwitchToReceive(void)
{ 
  if(LogAudio)
  { time_t Now;
    time(&Now);
    char TimeStr[32];
    AsciiTime(TimeStr,&Now);
    char AudioFileName[32];
    sprintf(AudioFileName,"mfsk_%s.log", TimeStr);
    if(Sound.OpenForRead(DeviceName,SampleRate,AudioFileName)<0) return -1; }
  else
  { if(Sound.OpenForRead(DeviceName,SampleRate)<0) return -1; }

  Receiver.Reset();
  Terminal.RxStr("\nReceiving ...\n");
  Transmit=0;

  return 0; }

// switch to transmit mode
static int SwitchToTransmit(void)
{ FlushReceiver();                           // flush the receiver and print the decoded text
  if(Sound.OpenForWrite(DeviceName,SampleRate)<0) return -1; // open the sound card in write mode
  Transmitter.Start();                       // start the transmitter
  Terminal.RxStr("\nTransmitting ...\n");    // write the message
  Transmit=1;                                // set mode = transmit
  return 0; }

// read the audio output of the Transmitter and write it into the soundcard
static int ReadTransmitterOutput(void)
{ uint8_t Char;
  if(Transmitter.GetChar(Char)>0)      // monitor the characters being transmitted
    Terminal.RxChar_Filtered(Char);    // and write them to the Rx window
  int Len=Transmitter.Output(TxBuffer); // read audio from the transmitter
  if(Sound.Write(TxBuffer,Len)<Len) return -1; // write the audio into the soundcard
  return Len; }

// ================================================================================

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
        { case 'l':
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
    { Help=1; }
  }

  if(Help)
  { printf("\n\
mfsk_trx [options]\n\
 options:\n\
  -d<device>            the soundcard device number or name [/dev/dsp]\n\
  -l                    log the decoded text to a file\n\
  -L                    log the received audio to a file\n\
"         );
    printf("%s\n",Parameters.OptionHelp());
    return -1; }

  time_t Now;
  time(&Now);
  char TimeStr[32];
  AsciiTime(TimeStr,&Now);

  Error=Parameters.Preset();
  if(Error<0)
  { printf("Parameters.Preset() => %d\n",Error); return -1; }

  // preset the Transmitter internal arrays
  Error=Transmitter.Preset(&Parameters);
  if(Error<0)
  { printf("Transmitter.Preset() => %d\n",Error); return -1; }

  // preset the Receiver internal arrays
  Error=Receiver.Preset(&Parameters);
  if(Error<0)
  { printf("Receiver.Preset() => %d\n",Error); return -1; }

  // see the audio block length required by the transmitter
  TxBufferLen=Transmitter.MaxOutputLen;

  // allocate the audio buffer accordingly
  AllocArray(&TxBuffer,TxBufferLen);

  sprintf(TextFileName,"mfsk_%s.log", TimeStr); // make the text log file name
  Error=Terminal.Preset(10,LogText ? TextFileName:0); // open the terminal
  if(Error<0)
  { printf("Terminal.Preset() => %d\n",Error); return -1; }

  SwitchToReceive();
  PrintReceiverMode();

  Terminal.TxStatUpp("Type your text below, Ctrl-T = Transmit, Ctrl-R = Receive, Ctrl-X = eXit");
  Terminal.TxStatLow("MFSK/Olivia Tx/Rx, Pawel Jalocha, March 2006");

  for( ; ; ) // the main program loop
  {
    for( ; ReadKeyboard()>0; ) ; // read and process the keyboard
    if(Transmit) // when transmit mode
	{ // read the audio out of the transmitter and write it into the sound device
	  if(ReadTransmitterOutput()<0) break; // if problems with the audio, break the loop
      if(ReceiveReq)
      { Transmitter.Stop();
	    ReceiveReq=0; }
      if(Transmitter.Running()==0)
	  { SwitchToReceive();
	    TransmitReq=0; }
	}
    else // when receive mode
    { if(FeedReceiver()<0) break;        // read the audio and feed it into the receiver
      for( ; ReadReceiverOutput()>0; ) ; // put decoded characters onto the terminal
      PrintReceiverStatus();              // update the receiver status
      if(ExitReq)
      { FlushReceiver();
        PrintReceiverStatus();
		break; }
      if(TransmitReq)
	  { SwitchToTransmit();
	    TransmitReq=0;
		ReceiveReq=0; }
    }

  }

  Terminal.Close(); // close the terminal

  free(TxBuffer);   // deallocate the transmitter audio buffer

  Sound.Close();    // close the audio device

  return 0; }
