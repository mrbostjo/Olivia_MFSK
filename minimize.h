
#ifndef __MINIMIZE_H__
#define __MINIMIZE_H__

#include <math.h>

#include "struc.h"

class MinSearch
{

  public:

   Seq<double> Parm;	    // parameter (current) values
   Seq<double> ParmLow;     // parameter lower range
   Seq<double> ParmUpp;     // parameter upper range
   Seq<double> ParmStep;    // parameter step
   Seq<double> ParmMaxStep; // parameter max. step

   double (*Func)(double *Parm, size_t ParmNum); // function to be minimized

   double FuncValue;	    // function value for given parameters

   MinSearch()
     { }

   ~MinSearch()
     { }

   void Free(void)
     { Parm.Free(); ParmLow.Free(); ParmUpp.Free();
       ParmStep.Free(); ParmMaxStep.Free(); }

   // preset for given fuction to minimize and number of parameters
   int Preset(double (*NewFunc)(double *, size_t), size_t ParmNum=0)
     { if(Parm.SetLen(ParmNum)<0) goto Error;
       if(ParmLow.SetLen(ParmNum)<0) goto Error;
       if(ParmUpp.SetLen(ParmNum)<0) goto Error;
       if(ParmStep.SetLen(ParmNum)<0) goto Error;
       if(ParmMaxStep.SetLen(ParmNum)<0) goto Error;
       Func=NewFunc; return 0;
       Error: Free(); return -1; }

   // add one more parameter, with given initial value, range, step.
   int AddParm(double Val, double Low=0.0, double Upp=1.0,
                           double Step=0, double MaxStep=0)
   { if(Parm.Join(Val)<0) goto Error;
     if(ParmLow.Join(Low)<0) goto Error;
     if(ParmUpp.Join(Upp)<0) goto Error;
     if(ParmStep.Join(Step)<0) goto Error;
     if(ParmMaxStep.Join(MaxStep)<0) goto Error;
     return 0;
     Error: Free(); return -1; }

   // call the function for current parameter values
   // and store the outcome
   void FuncCall(void)
     { FuncValue=(*Func)(Parm.Elem,Parm.Len); }
     
   // search for a minimum on a grid taken on the parameter ranges
   int GridSearch(int GridDiv=2)
     { double Min; int Num;
       Seq<int> Idx; size_t i;
       Seq<double> MinParm;

       if(Idx.SetLen(Parm.Len)<0) return -1;
       if(MinParm.SetLen(Parm.Len)) return -1;
       for(i=0; i<Idx.Len; i++)
       { Idx[i]=0; ParmStep[i]=(ParmUpp[i]-ParmLow[i])/GridDiv; }

       Parm.Copy(ParmLow);
       FuncCall();
       Min=FuncValue; MinParm.Copy(Parm); Num=1;

       for( ; ; )
       { for(i=0; i<Idx.Len; i++)
         { Idx[i]++;
           if(Idx[i]<=GridDiv)
           { Parm[i]+=ParmStep[i]; break; }
           Idx[i]=0; Parm[i]=ParmLow[i];
         } if(i>=Idx.Len) break;
         FuncCall();
         if(FuncValue<Min)
         { Min=FuncValue; MinParm.Copy(Parm); }
         // { int p; printf("Parms:"); for(p=0; p<Parm.Len; p++) printf(" %8.6f",Parm[p]); printf(" => %10.8f\n",FuncValue); }
         Num++; }

       Parm.Copy(MinParm);
       FuncCall();
       ParmMaxStep.Copy(ParmStep);

       return Num; }
     // return the number of calls to the function being minimized
     // Parm contains the best parameter values, FuncValue = minimized function value

/*
   // set the parameter limits to given radius from the current values
   void LimitParmToSteps(double Steps=1.0)
     { size_t i;
       for(i=0; i<Parm.Len; i++)
       { ParmLow[i]=Parm[i]-Steps*ParmStep[i];
         ParmUpp[i]=Parm[i]+Steps*ParmStep[i]; }
     }
*/
/*
   // iterative search with poly-2 estimates
   int Poly2SearchIter(void)
     { size_t p,Num;
       double Left,Right;
       double A,B,C,Delta;
       
       for(Num=0,p=0; p<Parm.Len; p++)
       { Parm[p]+=ParmStep[p]; Right=(*Func)(Parm.Elem,Parm.Len);
         Parm[p]-=2*ParmStep[p]; Left=(*Func)(Parm.Elem,Parm.Len);
         Parm[p]+=ParmStep[p];
         C=FuncValue;
         B=(Right-Left)/2;
         A=(Left+Right)/2-FuncValue;
         if(A<=0)
         { if(B>0.0) Delta=(-1.0);
           else if(B<0.0) Delta=1.0;
           else Delta=0.0; }
         else
         { Delta=(-B)/A/2;
           if(Delta>1.0) Delta=1.0;
           else if(Delta<(-1.0)) Delta=-1.0; }
         Parm[p]+=ParmStep[p]*Delta;
         FuncCall();
         Delta=fabs(Delta);
         if(Delta>1.0) ParmStep[p]*=4;
         else if(Delta<0.1) ParmStep[p]*=0.4;
         else ParmStep[p]*=4*Delta;
       }

     return Num; }
*/
/*
   double ParmStepRMS(void)
     { size_t p; double Sum;
       for(Sum=0.0,p=0; p<Parm.Len; p++)
         Sum+=ParmStep[p]*ParmStep[p];
       return sqrt(Sum/Parm.Len); }
*/
   int VectorSearchIter(size_t MaxIter=10)
     { size_t p,iter;
       double Left,Right;
       double A,B,C,Delta;
       Seq<double> dx;
       if(dx.SetLen(Parm.Len)<0) return -1;

       for(p=0; p<Parm.Len; p++)
       { Parm[p]+=ParmStep[p];   Right=(*Func)(Parm.Elem,Parm.Len);
         Parm[p]-=2*ParmStep[p]; Left=(*Func)(Parm.Elem,Parm.Len);
         Parm[p]+=ParmStep[p];
         C=FuncValue;
         B=(Right-Left)/2;
         A=(Left+Right)/2-FuncValue;
         if(A<=0)
         { if(B>0.0) Delta=(-1.0);
           else if(B<0.0) Delta=1.0;
           else Delta=0.0; }
         else
         { Delta=(-B)/A/2;
           if(Delta>1.0) Delta=1.0;
           else if(Delta<(-1.0)) Delta=-1.0; }
         dx[p]=ParmStep[p]*Delta;
/*
         Delta=fabs(Delta);
         if(Delta>1.0) ParmStep[p]*=4;
         else if(Delta<0.1) ParmStep[p]*=0.4;
         else ParmStep[p]*=4*Delta;
*/
       }

       for(iter=0; iter<MaxIter; iter++)
       { for(p=0; p<Parm.Len; p++)
           Parm[p]+=dx[p];
         Right=(*Func)(Parm.Elem,Parm.Len);
         for(p=0; p<Parm.Len; p++) Parm[p]-=2*dx[p];
         Left=(*Func)(Parm.Elem,Parm.Len);
         for(p=0; p<Parm.Len; p++) Parm[p]+=dx[p];
         C=FuncValue;
         B=(Right-Left)/2;
         A=(Left+Right)/2-FuncValue;
         if(A<=0)
         { if(B>0.0) Delta=(-2.0);
           else if(B<0.0) Delta=2.0;
           else Delta=0.0; }
         else
         { Delta=(-B)/A/2;
      if(Delta>4.0) Delta=4.0;
      else if(Delta<(-4.0)) Delta=-4.0; }
    for(p=0; p<Parm.Len; p++) Parm[p]+=Delta*dx[p];
    FuncCall();
    // if(fabs(Delta)<0.1) break;
  }
  return iter; }

} ;

#endif // of __MINIMIZE_H__
