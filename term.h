
#ifndef __TERM_H__
#define __TERM_H__

// ====================================================================
// simple, ASCII mode, split screen terminal

#include <stdio.h>
#include <string.h>

#include <ncurses.h>

class SplitTerm
{ public:

   int Init;                  // 1 => screen is initialized
   int Width, Height;         // screen size

   int RxPos, RxLen;          // Rx window position and size [row]
   int RxCurX, RxCurY, RxAct; // Rx Window cursor position and active flag
   unsigned char PrevRxChar; // for the Rx character filter

   int TxPos, TxLen;          // Tx window position and size [row]
   int TxCurX, TxCurY, TxAct; // Tx Window cursor position and active flag

   int StatPos[4];            // positions of the four status lines [row]

   FILE *LogFile;             // log file (NULL is not open)

  public:

   SplitTerm()
     { Init=0; LogFile=0; }

   ~SplitTerm()
     { Close(); }

   void Close(void)
     { if(Init)
	   { erase(); refresh(); endwin(); Init=0; }
	   if(LogFile)
	   { fclose(LogFile); LogFile=0; }
	 }

   // preset for given size of the transmitter (lower) window
   int Preset(int TxLines=0, char *LogFileName=0)
     {
       if(Init) endwin();

       initscr();
       cbreak();                // no buffering till EOL
       noecho();                // no echo
       nonl();
       nodelay(stdscr,TRUE);    // non-blocking getch() call
       // intrflush(stdscr,FALSE);
       keypad(stdscr,TRUE);     // function keys are being returned
       scrollok(stdscr,TRUE);   // scroll when cursor moves out
       erase();
       Init=1;

       Width=COLS; Height=LINES;
       TxLen=TxLines;
       if(TxLen==0) { RxLen=Height-TxLen-2; }
	           else { RxLen=Height-TxLen-4; }
       RxPos=1;
       TxPos=RxPos+RxLen+2;

       StatPos[0]=0;
       StatPos[1]=RxPos+RxLen;
       StatPos[2]=RxPos+RxLen+1;
       StatPos[3]=TxPos+TxLen;

       RxCurX=0; RxCurY=RxPos; RxAct=0;
       TxCurX=0; TxCurY=TxPos; TxAct=0;

       refresh();

       PrevRxChar=0;

	   if(LogFileName)
	     LogFile=fopen(LogFileName,"wt");

       return 0; }

   // get user input
   int UserInp(int &key)
     { key=getch(); return key==ERR ? 0 : 1; }

  private:

   void ChOut(unsigned char ch)
     {
       // if(ch=='\0') return;
       attrset(A_NORMAL);
       if((ch>=' ')||(ch=='\n')||(ch=='\b')||(ch=='\t'))
	   { addch(ch); }
	   // else if(ch=='\n')
	   // { clrtoeol(); addch(ch); clrtoeol(); 	}
       else { attrset(A_REVERSE); addch(ch+'@'); }
     }

  public:

   // put a character into the Receiver window but filter the control codes and LF/CR
   void RxChar_Filtered(unsigned char Char)
   {
     if(Char>=' ') { PrevRxChar=Char; RxChar(Char); return ; }

     if(Char=='\0') goto OutputNL;

     if((Char=='\n')||(Char=='\r'))
     { if(Char==PrevRxChar) goto OutputLF;
       if((PrevRxChar=='\n')||(PrevRxChar=='\r')) goto OutputNL;
	   goto OutputLF; }

     OutputNL: PrevRxChar=Char; return;

     OutputLF: PrevRxChar=Char; RxChar('\n');
   }

   // put a character into the Receiver window
   void RxChar(unsigned char Char)
     { if(!RxAct)
       { setscrreg(RxPos,RxPos+RxLen-1);
         move(RxCurY,RxCurX); RxAct=1; TxAct=0; }
       ChOut(Char); getyx(stdscr,RxCurY,RxCurX);
       refresh();
	   if(LogFile) fputc(Char,LogFile);
	 }

   // put a string into the Receiver window
   void RxStr(char *str)
     { if(!RxAct)
       { setscrreg(RxPos,RxPos+RxLen-1);
         move(RxCurY,RxCurX); RxAct=1; TxAct=0; }
       attrset(A_NORMAL); addstr(str);
       getyx(stdscr,RxCurY,RxCurX);
       refresh();
	   if(LogFile) fputs(str,LogFile);
	 }

   // put a character into the transmitter window
   void TxChar(unsigned char Char)
     { if(!TxAct)
       { setscrreg(TxPos,TxPos+TxLen-1);
         move(TxCurY,TxCurX); TxAct=1; RxAct=0; }
       ChOut(Char); getyx(stdscr,TxCurY,TxCurX);
       refresh(); }

  private:

   void Status(int Stat, char *Str)
     { int i,y=StatPos[Stat];
       RxAct=0; TxAct=0; move(y,0);
       attrset(A_REVERSE);
       for(i=0; (i<Width-1)&&Str[i]; i++) addch(Str[i]);
       for(   ; i<Width-1; i++) addch(' ');
       refresh(); }

  public:

   // put given string into given status line (there are four status lines)

   void RxStatUpp(char *Str) // upper receiver status
     { Status(0,Str); }

   void RxStatLow(char *Str) // lower receiver status
     { Status(1,Str); }

   void TxStatUpp(char *Str) // upper transmitter status
     { Status(2,Str); }
     
   void TxStatLow(char *Str) // lower transmitter status
     { Status(3,Str); }

} ;

// ====================================================================

#endif // of __TERM_H__
