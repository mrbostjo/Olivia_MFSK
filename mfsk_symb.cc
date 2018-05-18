// MTFSK symbol shape minimizer/calculator
// (c) 2004 Pawel Jalocha

#include <stdio.h>

#include "firgen.h"
#include "minimize.h"

FirGen FirGen;

int SymbolSepar;
int DataCarrSepar;

void SymbolCrossTalk(int &Points, double &TotalPower, double &PeakPower,
		     double *Parm, int ParmNum)
{ double Total,CrossTalk,Peak; int p,odd; size_t T,F;
  int SymbolSepar2=SymbolSepar/2;

  double Last=FirGen.FreqShape[0]=1.0;
  for(p=0; p<ParmNum; p++)
  { Last=(-Last);
    Last+=FirGen.FreqShape[1+p]=Parm[p];
  } FirGen.FreqShape[1+p]=Last;

  FirGen.MakeTimeShape();
  for(Points=0,Peak=Total=0.0,T=SymbolSepar; T<FirGen.TimeShape.Len; T+=SymbolSepar)
  { CrossTalk=FirGen.CrossTalkEnergy(T);
    Total+=2*CrossTalk; Points+=2;
    if(CrossTalk>Peak) Peak=CrossTalk; }

  for(F=DataCarrSepar,odd=1; F<(FirGen.CosineTable.Len/2); F+=DataCarrSepar,odd^=1)
  { FirGen.MakeWaveShape(F);

    { T=0; CrossTalk=FirGen.WaveCrossTalkEnergy(T);
      Total+=2*CrossTalk; Points+=2;
      if(CrossTalk>Peak) Peak=CrossTalk;
      // T+=SymbolSepar;
      for( T=SymbolSepar; T<FirGen.TimeShape.Len; T+=SymbolSepar)
      { CrossTalk=FirGen.WaveCrossTalkEnergy(T);
	    Total+=4*CrossTalk; Points+=4;
	    if(CrossTalk>Peak) Peak=CrossTalk; }
    }

  }
  CrossTalk=FirGen.CrossTalkEnergy(0);
  TotalPower=Total/CrossTalk; PeakPower=Peak/CrossTalk; }

double OnlyCrossTalk(double *Parm, size_t ParmNum)
{ int Points; double Total,Peak;
  SymbolCrossTalk(Points,Total,Peak,Parm,ParmNum);
  return Total+10*Peak; }

double CrossTalkWithSideLobes(double *Parm, size_t ParmNum)
{ int Points; double Total,Peak,Ret;
  SymbolCrossTalk(Points,Total,Peak,Parm,ParmNum);
  Ret=Total+10*Peak;
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  Ret+=2*(Total+10*Peak);
  return Ret; }

MinSearch MinSearch;

int main(int argc, char *argv[])
{ int Iter; int Points; double Total,Peak;

  if(FirGen.Preset(1024,2)) return 1;

  SymbolSepar=256; DataCarrSepar=4*FirGen.FreqGrid;

  { float SampleRate=8000;
    float DataCarriers=16;
    float CarrierWidth=(DataCarrSepar/FirGen.FreqGrid)*SampleRate/FirGen.TimeShape.Len;
    float SymbolRate=SampleRate/SymbolSepar;
    float BitRate=SymbolRate*log(DataCarriers)/log(2);
    printf("%4.2f Hz/carrier, %4.2f Hz Total, %4.2f Baud, %4.2f bps\n",
           CarrierWidth, CarrierWidth*DataCarriers, SymbolRate, BitRate);
  }

  if(MinSearch.Preset(OnlyCrossTalk,0)) return 1;
  if(MinSearch.AddParm(2.0,1.0,3.0)) return 1;
  if(MinSearch.AddParm(1.0,0.0,2.0)) return 1;

  MinSearch.GridSearch(16);
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power=%8.6f, peak=%8.6f (%d points)\n",Total,Peak,Points);
  FirGen.PrintFreqShape();

  FirGen.PrintCrossTalk(SymbolSepar/2,9,FirGen.FreqGrid,17,1);

  while(MinSearch.Parm.Len<2)
  { if(MinSearch.AddParm(0.0,-1.0,3.0,0.1)) return 1;
    for(Iter=0; Iter<20; Iter++) MinSearch.VectorSearchIter();
    SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
    printf("Total crosstalk power=%8.6f, peak=%8.6f (%d points)\n",Total,Peak,Points);
    FirGen.PrintFreqShape(); }

  for(Iter=0; Iter<50; Iter++) MinSearch.VectorSearchIter();
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power =%8.6f, peak=%8.6f (%d points)\n",Total,Peak,Points);
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  printf("Sidelobes power: total=%8.6f, peak=%8.6f\n",Total,Peak);
  FirGen.PrintFreqShape();

  MinSearch.Func=CrossTalkWithSideLobes;
  for(Iter=0; Iter<50; Iter++) MinSearch.VectorSearchIter();
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power =%8.6f, peak=%8.6f (%d points)\n",Total,Peak,Points);
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  printf("Sidelobes power: total=%8.6f, peak=%8.6f\n",Total,Peak);
  FirGen.PrintFreqShape(" %+11.8f");

  MinSearch.Func=CrossTalkWithSideLobes;
  for(Iter=0; Iter<50; Iter++) MinSearch.VectorSearchIter();
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power =%10.8f, peak=%10.8f (%d points)\n",Total,Peak,Points);
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  printf("Sidelobes power: total=%10.8f, peak=%10.8f\n",Total,Peak);
  FirGen.PrintFreqShape(" %+11.8f");

  MinSearch.Func=CrossTalkWithSideLobes;
  for(Iter=0; Iter<50; Iter++) MinSearch.VectorSearchIter();
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power =%10.8f, peak=%10.8f (%d points)\n",Total,Peak,Points);
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  printf("Sidelobes power: total=%10.8f, peak=%10.8f\n",Total,Peak);
  FirGen.PrintFreqShape(" %+13.10f");

  MinSearch.Func=CrossTalkWithSideLobes;
  for(Iter=0; Iter<50; Iter++) MinSearch.VectorSearchIter();
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power =%10.8f, peak=%10.8f (%d points)\n",Total,Peak,Points);
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  printf("Sidelobes power: total=%10.8f, peak=%10.8f\n",Total,Peak);
  FirGen.PrintFreqShape(" %+13.10f");

  MinSearch.Func=CrossTalkWithSideLobes;
  for(Iter=0; Iter<50; Iter++) MinSearch.VectorSearchIter();
  SymbolCrossTalk(Points,Total,Peak,MinSearch.Parm.Elem,MinSearch.Parm.Len);
  printf("Total crosstalk power =%10.8f, peak=%10.8f (%d points)\n",Total,Peak,Points);
  FirGen.RespDev(Total,Peak,2*DataCarrSepar,FirGen.CosineTable.Len/2);
  printf("Sidelobes power: total=%10.8f, peak=%10.8f\n",Total,Peak);
  FirGen.PrintFreqShape(" %+13.10f");

  // FirGen.PrintCrossTalk(SymbolSepar/2,8,FirGen.FreqGrid,16,0);
  FirGen.PrintCrossTalk(SymbolSepar/2,9,FirGen.FreqGrid,17,1);

  // FirGen.WriteShapeTable("const float Shape");

  return 1; }
