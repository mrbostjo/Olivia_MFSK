
#ifndef __FIRGEN_H__
#define __FIRGEN_H__

#include <stdio.h>
#include <math.h>

#include "struc.h"

class FirGen // symetric, real-valued FIR filter generator
{
  public:

   int FreqGrid;	        // frequency domain sub-grid factor

   Seq<double> FreqShape;   // the frequency-domain shape  [0..TimeShape.Len/2]
   Seq<double> CosineTable; // Cosine table to speed up trygonometrical fuctions
   Seq<double> TimeShape;   // the time-domain shape       [0..TimeShape.Len-1]
   Seq<double> WaveShapeI;  // TimeShape multiplied by a wave at selected frequency
   Seq<double> WaveShapeQ;

   FirGen()
     { FreqGrid=0; }

   ~FirGen()
     { }

   void Free(void)
     { CosineTable.Free(); TimeShape.Free(); FreqShape.Free();
       WaveShapeI.Free(); WaveShapeQ.Free(); FreqGrid=0; }

   // preset for given time-domain length and grid
   // Grid=2 (default) means that we intend to test the filter response
   // at frequency intervals = SampleRate/Len/Grid
   // Note: response at intervals = SampleRate/Len is defined by FreqShape
   int Preset(int Len, int Grid=2)
   { size_t t,Len4,Len2;
     if(Len&3) goto Error;   // Len must be a multiple of 4
     if(FreqShape.SetLen(Len/2+1)<0) goto Error;
     if(CosineTable.SetLen(Grid*Len)<0) goto Error;
     if(TimeShape.SetLen(Len)<0) goto Error;
     if(WaveShapeI.SetLen(Len)<0) goto Error;
     if(WaveShapeQ.SetLen(Len)<0) goto Error;
     Len4=CosineTable.Len/4; Len2=CosineTable.Len/2;
     // fill the wave table
     for(t=0; t<Len4; t++) CosineTable[t]=cos(t*(M_PI/Len2));
     CosineTable[t++]=0.0;
     for(   ; t<Len2; t++) CosineTable[t]=(-CosineTable[Len2-t]);
     for(   ; t<CosineTable.Len; t++) CosineTable[t]=(-CosineTable[t-Len2]);
     ClearFreqShape(); FreqGrid=Grid; return 0;
   Error: Free(); return -1; }

   // clear (set to all-zero) the freq.-domain shape
   void ClearFreqShape(void)
     { size_t f;
       for(f=0; f<FreqShape.Len; f++)
         FreqShape[f]=0.0; }

   // compute the time-domain shape based on the freq. domain shape
   void MakeTimeShape(void)
   { size_t t,t2,f,f2; double F;
     F=FreqShape[0]; for(t=0; t<TimeShape.Len; t++) TimeShape[t]=F;
     for(f=1; f<FreqShape.Len; f++)
     { F=FreqShape[f]; if(F==0.0) continue;
       if(f&1) F=(-F);
       for(f2=FreqGrid*f,t2=0,t=0; t<TimeShape.Len; t++)
       { TimeShape[t]+=F*CosineTable[t2];
         t2+=f2; if(t2>=CosineTable.Len) t2-=CosineTable.Len; }
     }
   }

   // compute WaveShapeI/Q for given TimeShape and grid Freq-quency
   void MakeWaveShape(int Freq)
   { int t,t2,t2q;
     for(t2=0,t=TimeShape.Len/2; t<(int)TimeShape.Len; t++)
     { t2q=t2-CosineTable.Len/4; if(t2q<0) t2q+=CosineTable.Len;
       WaveShapeI[t]=TimeShape[t]*CosineTable[t2]; WaveShapeQ[t]=TimeShape[t]*CosineTable[t2q];
       t2+=Freq; if(t2>=(int)CosineTable.Len) t2-=CosineTable.Len; }
     for(t2=0,t=TimeShape.Len/2-1; t>=0; t--)
     { t2-=Freq; if(t2<0) t2+=CosineTable.Len;
       t2q=t2-CosineTable.Len/4; if(t2q<0) t2q+=CosineTable.Len;
       WaveShapeI[t]=TimeShape[t]*CosineTable[t2]; WaveShapeQ[t]=TimeShape[t]*CosineTable[t2q]; }
   }

   // calculate the FIR shape response at given Freq-uency
   // the effective frequency is Freq*SampleRate
   // Note: this call is slow, because it computes the several co(sine) values
   double FreqResp(double Freq)
   { const double Pi2=2*M_PI;
     int t; double Resp,Phase;
     for(Resp=0,Phase=0,t=TimeShape.Len/2; t<(int)TimeShape.Len; t++)
     { Resp+=TimeShape[t]*cos(Phase);
       Phase+=Pi2*Freq; if(Phase>=M_PI) Phase-=Pi2; }
     for(Phase=0,t=TimeShape.Len/2-1; t>=0; t--)
     { Phase-=Pi2*Freq; if(Phase<M_PI) Phase+=Pi2;
       Resp+=TimeShape[t]*cos(Phase); }
     return Resp/TimeShape.Len; }

   // calculate the FIR shape response at given grid Freq-uency
   // the effective frequency is Freq*(SampleRate/Len/FreqGrid)
   // Note: this call is fast, as it uses the precalculated cosine table
   double GridFreqResp(int Freq)
   { int t,t2; double Resp;
     for(Resp=0,t2=0,t=TimeShape.Len/2; t<(int)TimeShape.Len; t++)
     { Resp+=TimeShape[t]*CosineTable[t2];
       t2+=Freq; if(t2>=(int)CosineTable.Len) t2-=CosineTable.Len; }
     for(t2=0,t=TimeShape.Len/2-1; t>=0; t--)
     { t2-=Freq; if(t2<0) t2+=CosineTable.Len;
       Resp+=TimeShape[t]*CosineTable[t2]; }
     return Resp/TimeShape.Len; }

   // get the highest response deviation in the given (grid) frequency range
   double PeakRespDev(int FreqLow, int FreqUpp, double RefResp=0)
   { int F; double Peak,Resp;
     for(Peak=0,F=FreqLow; F<=FreqUpp; F++)
     { Resp=GridFreqResp(F); Resp=fabs(Resp-RefResp);
       if(Resp>Peak) Peak=Resp; }
     return Peak; }

   // get the highest response deviation energy in the given grid-freq. range
   double PeakRespDevEnergy(int FreqLow, int FreqUpp, double RefResp=0)
   { double Peak=PeakRespDev(FreqLow,FreqUpp,RefResp);
     return Peak*Peak; }

   // get the summed response deviation energy for given grid-freq. range
   double RespDevEnergy(int FreqLow, int FreqUpp, double RefResp=0)
   { int F; double Energy,Resp;
     for(Energy=0,F=FreqLow; F<=FreqUpp; F++)
     { Resp=GridFreqResp(F); Resp=(Resp-RefResp); Energy+=Resp*Resp; }
     return Energy; }

   // get both: the highest response deviation and the total response dev. energy
   void RespDev(double &TotalEnergy, double &PeakEnergy, int FreqLow, int FreqUpp, double RefResp=0)
   { int F; double Total,Peak,Resp;
     for(Peak=Total=0.0,F=FreqLow; F<=FreqUpp; F++)
     { Resp=GridFreqResp(F); Resp=(Resp-RefResp); Resp=Resp*Resp;
       Total+=Resp; if(Resp>Peak) Peak=Resp; }
     TotalEnergy=Total; PeakEnergy=Peak; }
/*
   double CrossTalkEnergy(Seq<double> &Shape1, Seq<double> &Shape2, int TimeShift)
   { int t1,t2,len; double Resp;
     len=Shape2.Len-TimeShift; if(len>Shape1.Len) len=Shape1.Len;
     for(Resp=0,t1=0,t2=TimeShift; t1<len; t1++,t2++)
       Resp+=Shape1[t1]*Shape2[t2];
     return Resp*Resp; }
*/
   double CrossTalkEnergy(int TimeShift)
   { size_t t,t2; double Resp;
     for(Resp=0,t=0,t2=TimeShift; t2<TimeShape.Len; t++,t2++)
       Resp+=TimeShape[t]*TimeShape[t2];
     return Resp*Resp; }

   // calculate the crosstalk (ISI) between two shapes shifted in time and freq.
   double CrossTalkEnergy(int TimeShift, int FreqShift)
   { if(FreqShift==0) return CrossTalkEnergy(TimeShift);
     MakeWaveShape(FreqShift);
     return WaveCrossTalkEnergy(TimeShift); }

   double WaveCrossTalkEnergy(int TimeShift)
   { size_t t,t2; double RespI,RespQ;
     for(RespI=RespQ=0,t=0,t2=TimeShift; t2<TimeShape.Len; t++,t2++)
     { RespI+=TimeShape[t]*WaveShapeI[t2];
       RespQ+=TimeShape[t]*WaveShapeQ[t2]; }
     return RespI*RespI+RespQ*RespQ; }

   int WriteShapeTable(char *Table="double Shape", double Scale=1.0,
                       char *Form=" %+12.9f", FILE *File=stdout)
   { int t;
     if(fprintf(File,"\n%s[%d] = \n{ ",Table,TimeShape.Len)==EOF) return -1;
     for(t=0; t<(int)TimeShape.Len-1; t++)
     { if(t!=0) { if(fprintf(File,"  ")==EOF) return -1; }
       if(fprintf(File,Form,TimeShape[t]*Scale)==EOF) return -1;
       if(fprintf(File,",   // %4d\n",t)==EOF) return -1; }
     if(fprintf(File,"  ")==EOF) return -1;
     if(fprintf(File,Form,TimeShape[t]*Scale)==EOF) return -1;
     if(fprintf(File," }; // %4d\n",t)==EOF) return -1;
     if(fprintf(File,"\n")==EOF) return -1;
     return 0; }

   void PrintFreqShape(char *Form=" %+12.9f",FILE *File=stdout)
   { size_t F,Fmax;
     for(Fmax=FreqShape.Len-1; Fmax>0; Fmax--) if(FreqShape[Fmax]!=0.0) break;
     fprintf(File,"FreqShape: ");
     for(F=0; F<=Fmax; F++) fprintf(File,Form,FreqShape[F]);
     fprintf(File,"\n"); }

   void PrintResp(double SampleFreq=1.0, int dB=1, FILE *File=stdout)
   { size_t F; double Resp;
     for(F=0; F<=CosineTable.Len/2; F++)
     { fprintf(File,"%7.2f",F*(SampleFreq/CosineTable.Len));
       Resp=GridFreqResp(F); Resp=Resp*Resp;
       if(dB)
       { if(Resp>0.0) fprintf(File," %+6.1f dB\n",10.0*log10(Resp));
	             else fprintf(File," -INF.  dB\n");
       } else fprintf(File," %+6.4f\n",Resp);
     }
   }

   void PrintCrossTalk(int TimeStep, int TimeNum,
                       int FreqStep, int FreqNum,
                       int dB=1, FILE *File=stdout)
   { int T,Time,F,Freq; double Resp,Ref;
     Ref=CrossTalkEnergy(0);
     printf("Freq/Time");
     for(Time=T=0; T<TimeNum; T++,Time+=TimeStep) fprintf(File," %5d ",Time);
     fprintf(File,"\n");
     for(Freq=F=0; F<FreqNum; F++,Freq+=FreqStep)
     { fprintf(File,"%3d      ",Freq);
       for(Time=T=0; T<TimeNum; T++,Time+=TimeStep)
       { Resp=CrossTalkEnergy(Time,Freq);
         if(dB)
         { if(Resp>0.0) fprintf(File," %+6.1f",10.0*log10(Resp/Ref));
		   else fprintf(File," -INFIN");
         } else fprintf(File," %6.4f",Resp/Ref);
       } fprintf(File,"\n");
     }
   }

} ;

#endif // of __FIRGEN_H__

