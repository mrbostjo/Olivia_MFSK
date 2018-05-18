
#ifndef __NOISE_H__
#define __NOISE_H__

#include <stdlib.h>
#include <math.h>

#include "struc.h"
#include "cmpx.h"

inline double UniformNoise(void)
{ return ((double)rand()+1.0)/((double)RAND_MAX+1.0); }   

template <class Type>
 void WhiteNoise(Cmpx<Type> &Noise, Type Amplitude=1.0)
{ double Phase;
  Amplitude*=sqrt(-2.0*log(UniformNoise()));
  Phase=2*M_PI*UniformNoise();
  Noise.SetPhase(Phase,Amplitude); }

template <class Type>
 void WhiteNoise(Type *Noise, size_t Len, Type Amplitude=1.0)
{ size_t Idx; Cmpx<Type> CmpxNoise;
  for(Idx=0; Idx<Len; Idx++)
  { WhiteNoise(CmpxNoise,Amplitude);
    Noise[Idx]=CmpxNoise.Re; }
}

template <class Type>
 void AddWhiteNoise(Type *Noise, size_t Len, Type Amplitude=1.0)
{ size_t Idx; Cmpx<Type> CmpxNoise;
  for(Idx=0; Idx<Len; Idx++)
  { WhiteNoise(CmpxNoise,Amplitude);
    Noise[Idx]+=CmpxNoise.Re; }
}

template <class Type>
 void WhiteNoise(Cmpx<Type> *Noise, size_t Len, Type Amplitude=1.0)
{ size_t Idx;
  for(Idx=0; Idx<Len; Idx++)
    WhiteNoise(Noise[Idx],Amplitude);
}

template <class Type>
 void AddWhiteNoise(Cmpx<Type> *Input, size_t Len, Type Amplitude=1.0)
{ size_t Idx; Cmpx<Type> Noise;
  for(Idx=0; Idx<Len; Idx++)
  { WhiteNoise(Noise,Amplitude);
    Input[Idx]+=Noise; }
}

template <class Type>
 void AddWhiteNoise(Seq< Cmpx<Type> > &Input, Type Amplitude=1.0)
{ return AddWhiteNoise(Input.Elem, Input.Len, Amplitude); }

#endif // of __NOISE_H__
