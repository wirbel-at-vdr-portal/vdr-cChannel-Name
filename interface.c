/*
 * interface.c: Abstract user interface layer
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: interface.c 1.27 2000/11/01 11:25:25 kls Exp $
 */

#include "interface.h"
#include <ctype.h>
#include <unistd.h>

cInterface *Interface = NULL;

cInterface::cInterface(int SVDRPport)
{
  open = 0;
  cols[0] = 0;
  width = height = 0;
  keyFromWait = kNone;
  rcIo = NULL;
  SVDRP = NULL;
#if defined(REMOTE_RCU)
  rcIo = new cRcIoRCU("/dev/ttyS1");
#elif defined(REMOTE_LIRC)
  rcIo = new cRcIoLIRC("/dev/lircd");
#else
  rcIo = new cRcIoKBD;
#endif
  rcIo->SetCode(Keys.code, Keys.address);
  if (SVDRPport)
     SVDRP = new cSVDRP(SVDRPport);
}

cInterface::~cInterface()
{
  delete rcIo;
  delete SVDRP;
}

void cInterface::Open(int NumCols, int NumLines)
{
  if (!open++)
     cDvbApi::PrimaryDvbApi->Open(width = NumCols, height = NumLines);
}

void cInterface::Close(void)
{
  if (open == 1)
     Clear();
  if (!--open) {
     cDvbApi::PrimaryDvbApi->Close();
     width = height = 0;
     }
}

unsigned int cInterface::GetCh(bool Wait, bool *Repeat, bool *Release)
{
  if (open)
     cDvbApi::PrimaryDvbApi->Flush();
  if (!rcIo->InputAvailable())
     cFile::AnyFileReady(-1, Wait ? 1000 : 0);
  unsigned int Command;
  return rcIo->GetCommand(&Command, Repeat, Release) ? Command : 0;
}

eKeys cInterface::GetKey(bool Wait)
{
  if (SVDRP)
     SVDRP->Process();
  eKeys Key = keyFromWait;
  if (Key == kNone) {
     bool Repeat = false, Release = false;
     Key = Keys.Get(GetCh(Wait, &Repeat, &Release));
     if (Repeat)
        Key = eKeys(Key | k_Repeat);
     if (Release)
        Key = eKeys(Key | k_Release);
     }
  keyFromWait = kNone;
  return Key;
}

void cInterface::PutKey(eKeys Key)
{
  keyFromWait = Key;
}

eKeys cInterface::Wait(int Seconds, bool KeepChar)
{
  if (open)
     cDvbApi::PrimaryDvbApi->Flush();
  eKeys Key = kNone;
  time_t timeout = time(NULL) + Seconds;
  for (;;) {
      Key = GetKey();
      if ((Key != kNone && (RAWKEY(Key) != kOk || RAWKEY(Key) == Key)) || time(NULL) > timeout)
         break;
      }
  if (KeepChar && ISRAWKEY(Key))
     keyFromWait = Key;
  return Key;
}

void cInterface::Clear(void)
{
  if (open)
     cDvbApi::PrimaryDvbApi->Clear();
}

void cInterface::ClearEol(int x, int y, eDvbColor Color)
{
  if (open)
     cDvbApi::PrimaryDvbApi->ClrEol(x, y, Color);
}

void cInterface::SetCols(int *c)
{
  for (int i = 0; i < MaxCols; i++) {
      cols[i] = *c++;
      if (cols[i] == 0)
         break;
      }
}

char *cInterface::WrapText(const char *Text, int Width, int *Height)
{
  // Wraps the Text to make it fit into the area defined by the given Width
  // (which is given in character cells).
  // The actual number of lines resulting from this operation is returned in
  // Height.
  // The returned string is newly created on the heap and the caller
  // is responsible for deleting it once it is no longer used.
  // Wrapping is done by inserting the necessary number of newline
  // characters into the string.

  int Lines = 1;
  char *t = strdup(Text);
  char *Blank = NULL;
  char *Delim = NULL;
  int w = 0;

  Width *= cDvbApi::PrimaryDvbApi->CellWidth();

  while (*t && t[strlen(t) - 1] == '\n')
        t[strlen(t) - 1] = 0; // skips trailing newlines

  for (char *p = t; *p; ) {
      if (*p == '\n') {
         Lines++;
         w = 0;
         Blank = Delim = NULL;
         p++;
         continue;
         }
      else if (isspace(*p))
         Blank = p;
      int cw = cDvbApi::PrimaryDvbApi->Width(*p);
      if (w + cw > Width) {
         if (Blank) {
            *Blank = '\n';
            p = Blank;
            continue;
            }
         else {
            // Here's the ugly part, where we don't have any whitespace to
            // punch in a newline, so we need to make room for it:
            if (Delim)
               p = Delim + 1; // let's fall back to the most recent delimiter
            char *s = new char[strlen(t) + 2]; // The additional '\n' plus the terminating '\0'
            int l = p - t;
            strncpy(s, t, l);
            s[l] = '\n';
            strcpy(s + l + 1, p);
            delete t;
            t = s;
            p = t + l;
            continue;
            }
         }
      else
         w += cw;
      if (strchr("-.,:;!?_", *p)) {
         Delim = p;
         Blank = NULL;
         }
      p++;
      }

  *Height = Lines;
  return t;
}

void cInterface::Write(int x, int y, const char *s, eDvbColor FgColor, eDvbColor BgColor)
{
  if (open)
     cDvbApi::PrimaryDvbApi->Text(x, y, s, FgColor, BgColor);
}

void cInterface::WriteText(int x, int y, const char *s, eDvbColor FgColor, eDvbColor BgColor)
{
  if (open) {
     ClearEol(x, y, BgColor);
     int col = 0;
     for (;;) {
         const char *t = strchr(s, '\t');
         const char *p = s;
         char buf[1000];
         if (t && col < MaxCols && cols[col] > 0) {
            unsigned int n = t - s;
            if (n >= sizeof(buf))
               n = sizeof(buf) - 1;
            strncpy(buf, s, n);
            buf[n] = 0;
            p = buf;
            s = t + 1;
            }
         Write(x, y, p, FgColor, BgColor);
         if (p == s)
            break;
         x += cols[col++];
         }
     }
}

void cInterface::Title(const char *s)
{
  int x = (Width() - strlen(s)) / 2;
  if (x < 0)
     x = 0;
  ClearEol(0, 0, clrCyan);
  Write(x, 0, s, clrBlack, clrCyan);
}

void cInterface::Status(const char *s, eDvbColor FgColor, eDvbColor BgColor)
{
  ClearEol(0, -3, s ? BgColor : clrBackground);
  if (s)
     Write(0, -3, s, FgColor, BgColor);
}

void cInterface::Info(const char *s)
{
  Open();
  isyslog(LOG_INFO, s);
  Status(s, clrWhite, clrGreen);
  Wait();
  Status(NULL);
  Close();
}

void cInterface::Error(const char *s)
{
  Open();
  esyslog(LOG_ERR, s);
  Status(s, clrWhite, clrRed);
  Wait();
  Status(NULL);
  Close();
}

bool cInterface::Confirm(const char *s)
{
  Open();
  isyslog(LOG_INFO, "confirm: %s", s);
  Status(s, clrBlack, clrGreen);
  bool result = Wait(10) == kOk;
  Status(NULL);
  Close();
  isyslog(LOG_INFO, "%sconfirmed", result ? "" : "not ");
  return result;
}

void cInterface::HelpButton(int Index, const char *Text, eDvbColor FgColor, eDvbColor BgColor)
{
  if (open && Text) {
     const int w = Width() / 4;
     int l = (w - strlen(Text)) / 2;
     if (l < 0)
        l = 0;
     cDvbApi::PrimaryDvbApi->Fill(Index * w, -1, w, 1, BgColor);
     cDvbApi::PrimaryDvbApi->Text(Index * w + l, -1, Text, FgColor, BgColor);
     }
}

void cInterface::Help(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  HelpButton(0, Red,    clrBlack, clrRed);
  HelpButton(1, Green,  clrBlack, clrGreen);
  HelpButton(2, Yellow, clrBlack, clrYellow);
  HelpButton(3, Blue,   clrWhite, clrBlue);
}

void cInterface::QueryKeys(void)
{
  Keys.Clear();
  Clear();
  WriteText(1, 1, "Learning Remote Control Keys");
  WriteText(1, 3, "Phase 1: Detecting RC code type");
  WriteText(1, 5, "Press any key on the RC unit");
  cDvbApi::PrimaryDvbApi->Flush();
#ifndef REMOTE_KBD
  unsigned char Code = 0;
  unsigned short Address;
#endif
  for (;;) {
#ifdef REMOTE_KBD
      if (GetCh())
         break;
#else
      //TODO on screen display...
      if (rcIo->DetectCode(&Code, &Address)) {
         Keys.code = Code;
         Keys.address = Address;
         WriteText(1, 5, "RC code detected!");
         WriteText(1, 6, "Do not press any key...");
         cDvbApi::PrimaryDvbApi->Flush();
         rcIo->Flush(3000);
         ClearEol(0, 5);
         ClearEol(0, 6);
         cDvbApi::PrimaryDvbApi->Flush();
         break;
         }
#endif
      }
  WriteText(1, 3, "Phase 2: Learning specific key codes");
  tKey *k = Keys.keys;
  while (k->type != kNone) {
        char *Prompt;
        asprintf(&Prompt, "Press key for '%s'", k->name);
        WriteText(1, 5, Prompt);
        delete Prompt;
        for (;;) {
            unsigned int ch = GetCh();
            if (ch != 0) {
               switch (Keys.Get(ch)) {
                 case kUp:   if (k > Keys.keys) {
                                k--;
                                break;
                                }
                 case kDown: if (k > Keys.keys + 1) {
                                WriteText(1, 5, "Press 'Up' to confirm");
                                WriteText(1, 6, "Press 'Down' to continue");
                                ClearEol(0, 7);
                                ClearEol(0, 8);
                                for (;;) {
                                    eKeys key = GetKey();
                                    if (key == kUp) {
                                       Clear();
                                       return;
                                       }
                                    else if (key == kDown) {
                                       ClearEol(0, 6);
                                       break;
                                       }
                                    }
                                break;
                                }
                 case kNone: k->code = ch;
                             k++;
                             break;
                 default:    break;
                 }
               break;
               }
            }
        if (k > Keys.keys)
           WriteText(1, 7, "(press 'Up' to go back)");
        else
           ClearEol(0, 7);
        if (k > Keys.keys + 1)
           WriteText(1, 8, "(press 'Down' to end key definition)");
        else
           ClearEol(0, 8);
        }
}

void cInterface::LearnKeys(void)
{
  isyslog(LOG_INFO, "learning keys");
  Open();
  for (;;) {
      Clear();
      QueryKeys();
      Clear();
      WriteText(1, 1, "Learning Remote Control Keys");
      WriteText(1, 3, "Phase 3: Saving key codes");
      WriteText(1, 5, "Press 'Up' to save, 'Down' to cancel");
      for (;;) {
          eKeys key = GetKey();
          if (key == kUp) {
             Keys.Save();
             Close();
             return;
             }
          else if (key == kDown) {
             Keys.Load();
             Close();
             return;
             }
          }
      }
}

eKeys cInterface::DisplayChannel(int Number, const char *Name, bool WithInfo)
{
  // Number = 0 is used for channel group display and no EIT
  if (Number)
     rcIo->Number(Number);
  if (Name && !Recording()) {
     Open(MenuColumns, 5);
     cDvbApi::PrimaryDvbApi->Fill(0, 0, MenuColumns, 1, clrBackground);
     int BufSize = MenuColumns + 1;
     char buffer[BufSize];
     if (Number)
        snprintf(buffer, BufSize, "%d  %s", Number, Name ? Name : "");
     else
        snprintf(buffer, BufSize, "%s", Name ? Name : "");
     Write(0, 0, buffer);
     time_t t = time(NULL);
     struct tm *now = localtime(&t);
     snprintf(buffer, BufSize, "%02d:%02d", now->tm_hour, now->tm_min);
     Write(-5, 0, buffer);
     cDvbApi::PrimaryDvbApi->Flush();

#define INFO_TIMEOUT 5

     const cEventInfo *Present = NULL, *Following = NULL;

     int Tries = 0;
     if (Number && WithInfo) {
        for (; Tries < INFO_TIMEOUT; Tries++) {
            {
              cThreadLock ThreadLock;
              const cSchedules *Schedules = cDvbApi::PrimaryDvbApi->Schedules(&ThreadLock);
              if (Schedules) {
                 const cSchedule *Schedule = Schedules->GetSchedule();
                 if (Schedule) {
                    const char *PresentTitle = NULL, *PresentSubtitle = NULL, *FollowingTitle = NULL, *FollowingSubtitle = NULL;
                    int Lines = 0;
                    if ((Present = Schedule->GetPresentEvent()) != NULL) {
                       PresentTitle = Present->GetTitle();
                       if (!isempty(PresentTitle))
                          Lines++;
                       PresentSubtitle = Present->GetSubtitle();
                       if (!isempty(PresentSubtitle))
                          Lines++;
                       }
                    if ((Following = Schedule->GetFollowingEvent()) != NULL) {
                       FollowingTitle = Following->GetTitle();
                       if (!isempty(FollowingTitle))
                          Lines++;
                       FollowingSubtitle = Following->GetSubtitle();
                       if (!isempty(FollowingSubtitle))
                          Lines++;
                       }
                    if (Lines > 0) {
                       const int t = 6;
                       int l = 1;
                       cDvbApi::PrimaryDvbApi->Fill(0, 1, MenuColumns, Lines, clrBackground);
                       if (!isempty(PresentTitle)) {
                          Write(0, l, Present->GetTimeString(), clrYellow, clrBackground);
                          Write(t, l, PresentTitle, clrCyan, clrBackground);
                          l++;
                          }
                       if (!isempty(PresentSubtitle)) {
                          Write(t, l, PresentSubtitle, clrCyan, clrBackground);
                          l++;
                          }
                       if (!isempty(FollowingTitle)) {
                          Write(0, l, Following->GetTimeString(), clrYellow, clrBackground);
                          Write(t, l, FollowingTitle, clrCyan, clrBackground);
                          l++;
                          }
                       if (!isempty(FollowingSubtitle)) {
                          Write(t, l, FollowingSubtitle, clrCyan, clrBackground);
                          }
                       cDvbApi::PrimaryDvbApi->Flush();
                       if (Lines == 4) {
                          Tries = 0;
                          break;
                          }
                       }
                    }
                 }
            }
            eKeys Key = Wait(1, true);
            if (Key != kNone)
               break;
            }
        }
     eKeys Key = Wait(INFO_TIMEOUT - Tries, true);
     Close();
     if (Key == kOk)
        GetKey();
     if (Key == kGreen || Key == kYellow) {
        GetKey();
        do {
           Key = DisplayDescription((Key == kGreen) ? Present : Following);
           } while (Key == kGreen || Key == kYellow);
        Key = kNone;
        }
     return Key;
     }
  return kNone;
}

eKeys cInterface::DisplayDescription(const cEventInfo *EventInfo)
{
  eKeys Key = kNone;

  if (EventInfo) {
     int line = 0;

     Open();
     Clear();

     char buffer[MenuColumns + 1];
     snprintf(buffer, sizeof(buffer), "%s  %s", EventInfo->GetDate() ? EventInfo->GetDate() : "", EventInfo->GetTimeString() ? EventInfo->GetTimeString() : "");
     Write(-strlen(buffer), line, buffer, clrYellow);

     line = WriteParagraph(line, EventInfo->GetTitle());
     line = WriteParagraph(line, EventInfo->GetSubtitle());
     line = WriteParagraph(line, EventInfo->GetExtendedDescription());

     Key = Wait(300);
     Close();
     }
  return Key;
}

int cInterface::WriteParagraph(int Line, const char *Text)
{
  if (Line < Height() && Text) {
     Line++;
     char *s = strdup(Text);
     char *pStart = s, *pEnd;
     char *pEndText = &s[strlen(s) - 1];

     while (pStart < pEndText) {
           if (strlen(pStart) > (unsigned)(MenuColumns - 2))
              pEnd = &pStart[MenuColumns - 2];
           else
              pEnd = &pStart[strlen(pStart)];

           while (*pEnd != 0 && *pEnd != ' ' && pEnd > pStart)
                 pEnd--;

           //XXX what if there are no blanks???
           //XXX need to scroll if text is longer
           *pEnd = 0;
           Write(1, Line++, pStart, clrCyan);
           if (Line >= Height())
              return Line;
           pStart = pEnd + 1;
           }
     }
  return Line;
}

void cInterface::DisplayRecording(int Index, bool On)
{
  rcIo->SetPoints(1 << Index, On);
}

bool cInterface::Recording(void)
{
  // This is located here because the Interface has to do with the "PrimaryDvbApi" anyway
  return cDvbApi::PrimaryDvbApi->Recording();
}
