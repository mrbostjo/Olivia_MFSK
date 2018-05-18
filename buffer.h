
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "struc.h"

// ============================================================

// a simple FIFO buffer
template <class Type>
 class FIFO
{ public:

   size_t Len;

  private:

   size_t ReadPtr;
   size_t WritePtr;
   Type *Data;

  public:

   FIFO()
     { Init(); }

   ~FIFO()
     { free(Data); }

   void Init(void)
     { Data=0; Len=0; }

   void Free(void)
     { free(Data); Data=0; Len=0; }

   void Reset(void)
     { ReadPtr=WritePtr=0; }

   void Clear(void)
     { ReadPtr=WritePtr; }

   int Preset(size_t NewLen)
     { Len=NewLen; return Preset(); }

   int Preset(void)
	 { if(ReallocArray(&Data,Len)<0) return -1;
       Reset(); return 0; }

   // increment the pointer (with wrapping around)
   void IncrPtr(size_t &Ptr, size_t Step=1)
     { Ptr+=Step; if(Ptr>=Len) Ptr-=Len; }

   // FIFO is full ?
   int Full(void)
     { size_t Ptr=WritePtr; IncrPtr(Ptr);
	   return (Ptr==ReadPtr); }

   // FIFO is empty ?
   int Empty(void)
     { return (ReadPtr==WritePtr); }

   // how many elements we can write = space left in the FIFO
   size_t WriteReady(void)
     { int Ready=ReadPtr-WritePtr;
	   if(Ready<=0) Ready+=Len;
	   return Ready-1; }

   // how many elements we can read = space taken in the FIFO
   size_t ReadReady(void)
     { int Ready=WritePtr-ReadPtr;
	   if(Ready<0) Ready+=Len;
	   return Ready; }

   // write a new element
   int Write(Type &NewData)
     { size_t Ptr=WritePtr; IncrPtr(Ptr);
	   if(Ptr==ReadPtr) return 0;
       Data[WritePtr]=NewData;
	   WritePtr=Ptr; return 1; }

   // read the oldest element
   int Read(Type &OldData)
     { if(ReadPtr==WritePtr) return 0;
	   OldData=Data[ReadPtr]; IncrPtr(ReadPtr); return 1; }

   // lookup data in the FIFO but without taking them out
   int Lookup(Type &OldData, size_t Offset=0)
     { size_t Ready=ReadReady(); if(Offset>=Ready) return 0;
	   size_t Ptr=ReadPtr; IncrPtr(Ptr,Offset);
       OldData=Data[Ptr]; return 1; }
} ;

// ============================================================

// A circular buffer to store history of data.
// Data may come as single numbers or in batches
// of fixed size (-> Width)
template <class Type>
 class CircularBuffer
{ public:

   size_t Width;  // input/output data width (row width)
   size_t Len;    // buffer length (column height)

  public:

   size_t Size;   // total size of the storage in the buffer
   size_t Ptr;    // current pointer (counts rows)
   Type  *Data;   // allocated storage

  public:
   CircularBuffer()
     { Init(); }

   ~CircularBuffer()
     { free(Data); }

   void Init(void)
     { Data=0; Size=0; Width=1; }

   void Free(void)
     { free(Data); Data=0; Size=0; }

   // reset: set pointer to the beginning of the buffer
   void Reset(void)
     { Ptr=0; }

   // preset for given length and width
   int Preset(size_t NewLen, size_t NewWidth=1)
     { Len=NewLen; Width=NewWidth; return Preset(); }

   int Preset(void)
     { Size=Width*Len;
       if(ReallocArray(&Data,Size)<0) return -1;
       Reset(); return 0; }

   // set all elements to given value
   void Set(Type &Value)
     { size_t Idx;
	   for(Idx=0; Idx<Size; Idx++)
	     Data[Idx]=Value;
	 }

   // set all elements to zero
   void Clear(void)
     { Type Zero; Zero=0; Set(Zero); }

   // increment the pointer (with wrapping around)
   void IncrPtr(size_t &Ptr, size_t Step=1)
     { Ptr+=Step; if(Ptr>=Len) Ptr-=Len; }

   // decrement the pointer (with wrapping around)
   void DecrPtr(size_t &Ptr, size_t Step=1)
     { if(Ptr>=Step) Ptr-=Step;
	            else Ptr+=(Len-Step); }

   template <class PhaseType>
    void WrapPhase(PhaseType &Phase)
     { if(Phase<0) Phase+=Len;
	   else if(Phase>=Len) Phase-=Len; }

   template <class PhaseType>
    void WrapDiffPhase(PhaseType &Phase)
     { if(Phase<(-(PhaseType)Len/2)) Phase+=Len;
	   else if(Phase>=((PhaseType)Len/2)) Phase-=Len; }

   // synchronize current pointer with another circular buffer
   template <class SrcType>
    void operator |= (CircularBuffer<SrcType> Buffer)
     { Ptr=Buffer.Ptr; }

   // advance (increment) current pointer
   void operator += (size_t Step)
     { IncrPtr(Ptr,Step); }

   // decrement current pointer
   void operator -= (size_t Step)
     { DecrPtr(Ptr,Step); }

   // index operator to get the absolute data pointer
   Type *operator [] (size_t Idx)
     { return Data+(Idx*Width); }

   // get the current pipe pointer
   Type *CurrPtr(void)
     { return Data+(Ptr*Width); }

   // get storage pointer relative to current pointer +/- offset
   Type *OffsetPtr(int Offset)
     { Offset+=Ptr; Offset*=Width;
	   if(Offset<0) Offset+=Size;
	   else if(Offset>=(int)Size) Offset-=Size;
	   return Data+Offset; }

} ;

// ============================================================

#endif // of __BUFFER_H__
