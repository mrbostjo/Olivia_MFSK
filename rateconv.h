
#ifndef __RATECONV_H__
#define __RATECONV_H__

#include "struc.h"


// =====================================================================
/*
// fast power of two
static inline size_t Exp2(uint32_t X)
{ return (uint32_t)1<<X; }

// fast base-2 logarythm
static inline size_t Log2(uint32_t X)
{ uint32_t Y;
  for( Y=0; X>1; X>>=1)
    Y+=1;
  return Y; }
*/
// =====================================================================

// rate converter, to correct for the soundcard sampling rate
template <class Type=float>
 class RateConverter
{ public:

   // parameters to be set by the user
   size_t TapLen;       // filter tap length (in term of input samples)
   size_t OverSampling; // internal oversampling factor
   Type UpperFreq;      // upper frequency of the (lowpass) filter (in terms of input sampling rate)
   Type OutputRate;     // the output rate (in terms of the input rate)

  private:

   size_t FilterLen;    // the total length of the filter (in term of oversampled rate)
   Type *FilterShape;   // the shape of the filter
   Type *InputTap;      // filter tap
   size_t InputTapPtr;
   size_t InputWrap;

   Type OutputTime;
   Type OutputPeriod;
   Type OutputBefore;
   Type OutputAfter;
   size_t OutputPtr;

  public:

   RateConverter()
     { Init();
	   Default(); }
	 
   ~RateConverter()
     { Free(); }

   void Init(void)
     { FilterShape=0;
	   InputTap=0; }

   void Free(void)
     { free(FilterShape); FilterShape=0;
	   free(InputTap); InputTap=0; }

   void Default(void)
     { TapLen=16;
       OverSampling=16;
       UpperFreq=3.0/8;
	   OutputRate=1.0; }

   int Preset(void)
     { size_t Idx;

       // TapLen=Exp2(Log2(TapLen));
       FilterLen=TapLen*OverSampling;

       if((ReallocArray(&FilterShape,FilterLen))<0) goto Error;
       if((ReallocArray(&InputTap,TapLen))<0) goto Error;

       for(Idx=0; Idx<FilterLen; Idx++)
       { Type Phase=(M_PI*(2*(int)Idx-(int)FilterLen))/FilterLen;
	     // Type Window=0.50+0.50*cos(Phase);                        // Hanning
	     // Type Window=0.42+0.50*cos(Phase)+0.08*cos(2*Phase);    // Blackman
	     Type Window=0.35875+0.48829*cos(Phase)+0.14128*cos(2*Phase)+0.01168*cos(3*Phase);    // Blackman-Harris
	     Type Filter=1.0;
		 if(Phase!=0)
		 { Phase*=(UpperFreq*TapLen); Filter=sin(Phase)/Phase; }
         // printf("%3d: %+9.6f %+9.6f %+9.6f\n", Idx, Window, Filter, Window*Filter);
		 FilterShape[Idx]=Window*Filter; }

       Reset();

	   return 0;
	   
	   Error: Free(); return -1; }

   void Reset(void)
     { size_t Idx;

       InputWrap=TapLen-1;
       for(Idx=0; Idx<TapLen; Idx++)
       { InputTap[Idx]=0; }
       InputTapPtr=0;

       OutputTime=0;
       OutputPeriod=OverSampling/OutputRate;
       OutputBefore=0;
	   OutputAfter=0;
       OutputPtr=0;	}

  private:
  
   Type Convolute(size_t Shift=0)
     { Type Sum=0;
       Shift=(OverSampling-1)-Shift;
	   size_t Idx=InputTapPtr;
	   for( ; Shift<FilterLen; Shift+=OverSampling)
       { Sum+=InputTap[Idx]*FilterShape[Shift];
	     Idx+=1; Idx&=InputWrap; }
	   return Sum; }

   void NewInput(Type Input)
     { // printf("I:\n");
	   InputTap[InputTapPtr]=Input;
	   InputTapPtr+=1; InputTapPtr&=InputWrap; }

  public:

   // process samples, store output at a pointer (you need to ensure enough storage)
   template <class InpType, class OutType>
    int Process(InpType *Input, size_t InputLen, OutType *Output)
     { size_t OutputLen=0;
	   // printf("E: %d %3.1f %d %d\n",OutputPtr, OutputTime, InputLen, OutputLen);
       for( ; ; )
	   { // printf("L: %d %3.1f %d %d\n",OutputPtr, OutputTime, InputLen, OutputLen);
	     if(OutputPtr)
	     { size_t Idx=(size_t)floor(OutputTime)+1;
           if(Idx>=OverSampling)
		   { if(InputLen==0) break;
		     NewInput(*Input); Input++; InputLen-=1;
             Idx-=OverSampling;
			 OutputTime-=(Type)OverSampling; }
           OutputAfter=Convolute(Idx);
		   Type Weight=Idx-OutputTime;
		   (*Output)=Weight*OutputBefore+(1.0-Weight)*OutputAfter;
           Output++;
		   OutputLen+=1;
	       // printf("O: %d %3.1f %d %d %d\n",OutputPtr, OutputTime, InputLen, OutputLen, Idx);
		   OutputPtr=0; }
		 else
	     { size_t Idx=(size_t)floor(OutputTime+OutputPeriod);
           if(Idx>=OverSampling)
		   { if(InputLen==0) break;
		     NewInput(*Input); Input++; InputLen-=1;
             Idx-=OverSampling;
			 OutputTime-=(Type)OverSampling; }
           OutputBefore=Convolute(Idx);
		   OutputTime+=OutputPeriod;
		   OutputPtr=1; }
	   }
	   // printf("R: %d %3.1f %d %d\n",OutputPtr, OutputTime, InputLen, OutputLen);
	   return OutputLen; }

   // process samples. store output in a Seq<> with automatic allocation
   template <class InpType, class OutType>
    int Process(InpType Input, size_t InputLen, Seq<OutType> &Output, int Append=1)
     { size_t OutPtr = Append ? Output.Len:0;
	   size_t MaxOutLen=(size_t)ceil(InputLen*OutputRate+2);
       if(Output.EnsureSpace(OutPtr+MaxOutLen)<0) return -1;
	   int OutLen=Process(Input, InputLen, Output.Elem+OutPtr);
       Output.Len=OutPtr+OutLen;
	   return OutLen; }

   template <class InpType, class OutType>
    int Process(Seq<InpType> &Input, Seq<OutType> &Output, int Append=1)
     { return Process(Input.Elem, Input.Len, Output, Append); }

   // process a single sample (returns the number of samples on the output)
   template <class InpType, class OutType>
    int Process(InpType Input, OutType *Output)
     { return Process(&Input,1,Output); }

} ;

// =====================================================================

#endif // of __RATECONV_H__
