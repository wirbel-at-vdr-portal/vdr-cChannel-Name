/*
 * svdrp.h: Simple Video Disk Recorder Protocol
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: svdrp.h 1.8 2001/04/01 14:10:33 kls Exp $
 */

#ifndef __SVDRP_H
#define __SVDRP_H

#include "dvbapi.h"
#include "tools.h"

class cSocket {
private:
  int port;
  int sock;
  int queue;
  void Close(void);
public:
  cSocket(int Port, int Queue = 1);
  ~cSocket();
  bool Open(void);
  int Accept(void);
  };

#define MAXCMDBUFFER 1024

class cSVDRP {
private:
  cSocket socket;
  cFile file;
  CRect ovlClipRects[MAXCLIPRECTS];
  uint numChars;
  char cmdLine[MAXCMDBUFFER];
  char *message;
  time_t lastActivity;
  void Close(bool Timeout = false);
  bool Send(const char *s, int length = -1);
  void Reply(int Code, const char *fmt, ...);
  void CmdCHAN(const char *Option);
  void CmdDELC(const char *Option);
  void CmdDELT(const char *Option);
  void CmdGRAB(const char *Option);
  void CmdHELP(const char *Option);
  void CmdHITK(const char *Option);
  void CmdLSTC(const char *Option);
  void CmdLSTT(const char *Option);
  void CmdMESG(const char *Option);
  void CmdMODC(const char *Option);
  void CmdMODT(const char *Option);
  void CmdMOVC(const char *Option);
  void CmdMOVT(const char *Option);
  void CmdNEWC(const char *Option);
  void CmdNEWT(const char *Option);
  void CmdOVLF(const char *Option);
  void CmdOVLG(const char *Option);
  void CmdOVLC(const char *Option);
  void CmdOVLP(const char *Option);
  void CmdOVLO(const char *Option);
  void CmdUPDT(const char *Option);
  void Execute(char *Cmd);
public:
  cSVDRP(int Port);
  ~cSVDRP();
  void Process(void);
  char *GetMessage(void);
  };

#endif //__SVDRP_H
