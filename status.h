/*
 * status.h: Status monitoring
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: status.h 1.1 2002/05/19 14:54:15 kls Exp $
 */

#ifndef __STATUS_H
#define __STATUS_H

#include "config.h"
#include "dvbapi.h"
#include "tools.h"

class cStatusMonitor : public cListObject {
private:
  static cList<cStatusMonitor> statusMonitors;
protected:
  // These functions can be implemented by derived classes to receive status information:
  virtual void ChannelSwitch(const cDvbApi *DvbApi, int ChannelNumber) {}
               // Indicates a channel switch on DVB device DvbApi.
               // If ChannelNumber is 0, this is before the channel is being switched,
               // otherwise ChannelNumber is the number of the channel that has been switched to.
  virtual void Recording(const cDvbApi *DvbApi, const char *Name) {}
               // DVB device DvbApi has started recording Name. Name is the full directory
               // name of the recording. If Name is NULL, the recording has ended. 
  virtual void Replaying(const cDvbApi *DvbApi, const char *Name) {}
               // DVB device DvbApi has started replaying Name. Name is the full directory
               // name of the recording. If Name is NULL, the replay has ended. 
  virtual void SetVolume(int Volume, bool Absolute) {}
               // The volume has been set to the given value, either
               // absolutely or relative to the current volume.
  virtual void OsdClear(void) {}
               // The OSD has been cleared.
  virtual void OsdTitle(const char *Title) {}
               // Title has been displayed in the title line of the menu.
  virtual void OsdStatusMessage(const char *Message) {}
               // Message has been displayed in the status line of the menu.
               // If Message is NULL, the status line has been cleared.
  virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue) {}
               // The help keys have been set to the given values (may be NULL).
  virtual void OsdCurrentItem(const char *Text) {}
               // The OSD displays the given single line Text as the current menu item.
  virtual void OsdTextItem(const char *Text, bool Scroll) {}
               // The OSD displays the given multi line text. If Text points to an
               // actual string, that text shall be displayed and Scroll has no
               // meaning. If Text is NULL, Scroll defines whether the previously
               // received text shall be scrolled up (true) or down (false) and
               // the text shall be redisplayed with the new offset.
  virtual void OsdChannel(const char *Text) {}
               // The OSD displays the single line Text with the current channel information.
  virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle) {}
               // The OSD displays the given programme information.
public:
  cStatusMonitor(void);
  virtual ~cStatusMonitor();
  // These functions are called whenever the related status information changes:
  static void MsgChannelSwitch(const cDvbApi *DvbApi, int ChannelNumber);
  static void MsgRecording(const cDvbApi *DvbApi, const char *Name);
  static void MsgReplaying(const cDvbApi *DvbApi, const char *Name);
  static void MsgSetVolume(int Volume, bool Absolute);
  static void MsgOsdClear(void);
  static void MsgOsdTitle(const char *Title);
  static void MsgOsdStatusMessage(const char *Message);
  static void MsgOsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
  static void MsgOsdCurrentItem(const char *Text);
  static void MsgOsdTextItem(const char *Text,  bool Scroll = false);
  static void MsgOsdChannel(const char *Text);
  static void MsgOsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);
  };

#endif //__STATUS_H
