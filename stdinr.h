
#ifndef __STDINR_H__
#define __STDINR_H__

// =====================================================================

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>

static struct termios Stdin_OrigState; // saved state of standard input

// set the standard input in "raw" mode so we can read single keystrokes
static inline int Stdin_SetRaw(void)
{ int err; struct termios Stdin_State;

  err=isatty(STDIN_FILENO);
  if(err==0)
  { // printf("STDIN is not a terminal ?\n");
    return -1; }
  err=fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  if(err)
  { // printf("Can't set the NONBLOCK option on STDIN\n");
    return -1; }
  err=tcgetattr(STDIN_FILENO,&Stdin_OrigState);
  if(err)
  { // printf("tcgetattr() returns %d on STDIN\n",err);
    return -1; }
  err=tcgetattr(STDIN_FILENO,&Stdin_State);
  if(err)
  { // printf("tcgetattr() returns %d on STDIN\n",err);
    return -1; }
  Stdin_State.c_lflag &= ~(ICANON|ECHO);
  Stdin_State.c_cc[VMIN] = 1;
  Stdin_State.c_cc[VTIME] = 0;
  // cfmakeraw(&Stdin_State);
  err=tcsetattr(STDIN_FILENO,TCSANOW,&Stdin_State);
  if(err)
  { // printf("tcsetattr() returns %d on STDIN\n",err);
    return -1; }
  return 0;
} //  0 => OK
  // -1 => problems...

// check if STDIN is ready for read - does NOT need to call StdinSetRaw()
static inline int Stdin_Ready(void)
{ fd_set InpSet;
  struct timeval Timeout;
  FD_ZERO(&InpSet); FD_SET(STDIN_FILENO,&InpSet);
  Timeout.tv_sec=0; Timeout.tv_usec=0;
  if(select(STDIN_FILENO+1,&InpSet,NULL,NULL,&Timeout)<0) return -1;
  return FD_ISSET(STDIN_FILENO,&InpSet);
} //  1 => STDIN has got something
  //  0 => nothing to read from STDIN
  // -1 => problems

// read a character from standard input
static inline int Stdin_Read(void)
{ int err; char key;
  err=read(STDIN_FILENO,&key,1);
  if(err<0) { return errno==EAGAIN ? 0 : -1; }
  if(err==0) return 0;
  return (int)key;
} //  0 => input is empty
  // >0 => key code
  // -1 => problems

// restore standard input to state as before StdinSetRaw()
static inline int Stdin_Restore(void)
{ int err=tcsetattr(STDIN_FILENO,TCSANOW,&Stdin_OrigState);
  if(err)
  { // printf("tcsetattr() returns %d on STDIN\n",err);
    return -1; }
  return 0; }

// =====================================================================

#endif // of __STDINR_H__













