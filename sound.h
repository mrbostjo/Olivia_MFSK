
// Sound input/output, for Linux or Cygwin audio interface
// (c) 1999-2005, Pawel Jalocha

#ifndef __SOUND_H__
#define __SOUND_H__

// ============================================================================

// #include <stdio.h>	// for debug only

#include <stdint.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <fcntl.h>
#include <errno.h>

// ============================================================================

class SoundDevice
{ public:

   int ReadDevNo;         // input device or file handle
   int ReadFromDev;       // true if we read from a device not a file
   int WriteDevNo;        // output device handle
   int WriteFileNo;       // output file handle
   int Rate;              // actuall sampling rate (reported by the device)

  public:

   SoundDevice()
     { ReadDevNo=(-1); WriteDevNo=(-1); WriteFileNo=(-1); }

   ~SoundDevice()
     { Close(); }

   // close the sound device (will wait until all written data is played)
   int Close(void)
     { int Err1,Err2,Err3;
       if(ReadDevNo>=0) { Err1=close(ReadDevNo); ReadDevNo=(-1); }
                   else { Err1=0; }
       if(WriteDevNo>=0) { Err2=close(WriteDevNo); WriteDevNo=(-1); }
                    else { Err2=0; }
       if(WriteFileNo>=0) { Err3=close(WriteFileNo); WriteFileNo=(-1); }
                     else { Err3=0; }
       return (Err1<0)||(Err2<0)||(Err3<0) ? -1 : 0; }

  private:
   // Open a sound device for read or write
   // For now we always open in 16-bit mono mode.
   int SoundDevice::Open(char *Device, int &SamplingRate, int Read)
     { int Err; int Param;

       int Mode = Read ? O_RDONLY:O_WRONLY;  // mode = read or write
       int Handle =open(Device,Mode,0);      // open device
       if(Handle<0) return -1;               // return if error
       // printf("open() OK.\n");

       Param=AFMT_S16_LE; // set sample format to 16-bit, signed, little endian
       Err=ioctl(Handle,SNDCTL_DSP_SETFMT,&Param);
       if((Err)||(Param!=AFMT_S16_LE)) { close(Handle); return -1; }
       // printf("Format selection OK.\n");

       Param=0; // select mono mode (that is turn the stereo mode OFF)
       Err=ioctl(Handle,SNDCTL_DSP_STEREO,&Param);
       if((Err)||(Param!=0)) { close(Handle); return -1; }
       // printf("Mono mode selection OK.\n");

       Param=SamplingRate; // request the sampling rate
       Err=ioctl(Handle,SNDCTL_DSP_SPEED,&Param);
       if(Err) { close(Handle); return -1; }
       SamplingRate=Param; // here the device tells us the actuall sampling rate
       // printf("Rate selection OK.\n");

       return Handle;

     } // returns the file handle or -1 if there are problems

  public:

   // open for reading the sound, optionally ask to save audio to a file
   int OpenForRead(char *Device="/dev/dsp", int ReqRate=8000,
                   char *SaveFile=0)
     { Close();
       Rate=ReqRate;
       ReadDevNo=Open(Device,Rate,1);
       if(ReadDevNo<0) return -1;
       ReadFromDev=1;
       if(SaveFile)
       { WriteFileNo=open(SaveFile,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
         if(WriteFileNo<0) { Close(); return -1; }
       }
       return ReadDevNo; } // returns the device file handle (>=0) or error (<0)

   // open to write the sound, optionally ask to save audio in a file
   int OpenForWrite(char *Device="/dev/dsp", int ReqRate=8000,
                    char *SaveFile=0)
     { Close();
       Rate=ReqRate;
       WriteDevNo=Open(Device,Rate,0);
       if(WriteDevNo<0) return -1;
       if(SaveFile)
       { WriteFileNo=open(SaveFile,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
         if(WriteFileNo<0) { Close(); return -1; }
       }
       return WriteDevNo; } // returns the device file handle (>=0) or error (<0)

   // open a file for read, optionally monitor the audio
   int OpenFileForRead(char *FileName, int FileRate=8000,
                       char *MonDevice=0)
     { Close();
       ReadDevNo=open(FileName,O_RDONLY);
       if(ReadDevNo<0) return -1;
       ReadFromDev=0;
       Rate=FileRate;
       if(MonDevice)
       { WriteDevNo=Open(MonDevice,FileRate,0);
         if(WriteDevNo<0) { Close(); return -1; } }
       return 0; }

   // open a file for write, optionally monitor the audio
   int OpenFileForWrite(char *FileName, int FileRate=8000,
                        char *MonDevice=0)
     { Close();
       WriteFileNo=open(FileName,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
       if(WriteFileNo<0) return -1;
       Rate=FileRate;
       if(MonDevice)
       { WriteDevNo=Open(MonDevice,FileRate,0);
         if(WriteDevNo<0) { Close(); return -1; } }
       return 0; }

   // read samples from an (open) device or file
   int Read(int16_t *Buffer, int Samples)
     { Samples<<=1;
       int ReadLen=read(ReadDevNo,Buffer,Samples);
       if(ReadLen!=Samples) return -1;
       if(WriteDevNo>=0)
       { int WriteLen=write(WriteDevNo,Buffer,Samples);
         if(WriteLen!=Samples) return -1; }
       if(WriteFileNo>=0)
       { int WriteLen=write(WriteFileNo,Buffer,Samples);
         if(WriteLen!=Samples) return -1; }
       ReadLen>>=1;
       return ReadLen; }

   // see how much data is in the input buffer
   int ReadReady(void)
     { int Err; audio_buf_info info;
       if(ReadFromDev)
       { Err=ioctl(ReadDevNo,SNDCTL_DSP_GETISPACE,&info);
         return Err ? -1 : info.bytes>>1; }
       else return 0x4000;
     } // returns the number of samples we can immediately read

   // write 16-bit samples into an (open) device / file
   int Write(int16_t *Buffer, int Samples)
     { Samples<<=1;
       if(WriteDevNo>=0)
       { int WriteLen=write(WriteDevNo,Buffer,Samples);
         if(WriteLen!=Samples) return -1; }
       if(WriteFileNo>=0)
       { int WriteLen=write(WriteFileNo,Buffer,Samples);
         if(WriteLen!=Samples) return -1; }
       Samples>>=1;
       return Samples; }

   // see how much space is in the output buffer
   int WriteReady(void)
     { int Err; audio_buf_info info;
       if(WriteDevNo>=0)
       { Err=ioctl(WriteDevNo,SNDCTL_DSP_GETOSPACE,&info);
         return Err ? -1 : info.bytes>>1; }
       else return 0x4000;
     } // returns the number of samples we can immediately write

} ;

// ============================================================================

#endif // of __SOUND_H__
