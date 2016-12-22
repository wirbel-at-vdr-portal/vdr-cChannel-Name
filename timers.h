/*
 * timers.h: Timer handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: timers.h 4.4 2016/12/22 14:23:50 kls Exp $
 */

#ifndef __TIMERS_H
#define __TIMERS_H

#include "channels.h"
#include "config.h"
#include "epg.h"
#include "tools.h"

enum eTimerFlags { tfNone      = 0x0000,
                   tfActive    = 0x0001,
                   tfInstant   = 0x0002,
                   tfVps       = 0x0004,
                   tfRecording = 0x0008,
                   tfAll       = 0xFFFF,
                 };
enum eTimerMatch { tmNone, tmPartial, tmFull };

class cTimer : public cListObject {
  friend class cMenuEditTimer;
private:
  int id;
  mutable time_t startTime, stopTime;
  int scheduleState;
  mutable time_t deferred; ///< Matches(time_t, ...) will return false if the current time is before this value
  bool pending, inVpsMargin;
  uint flags;
  const cChannel *channel;
  mutable time_t day; ///< midnight of the day this timer shall hit, or of the first day it shall hit in case of a repeating timer
  int weekdays;       ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
  int start;
  int stop;
  int priority;
  int lifetime;
  mutable char file[NAME_MAX * 2 + 1]; // *2 to be able to hold 'title' and 'episode', which can each be up to 255 characters long
  char *aux;
  char *remote;
  const cEvent *event;
public:
  cTimer(bool Instant = false, bool Pause = false, const cChannel *Channel = NULL);
  cTimer(const cEvent *Event);
  cTimer(const cTimer &Timer);
  virtual ~cTimer();
  cTimer& operator= (const cTimer &Timer);
  virtual int Compare(const cListObject &ListObject) const;
  int Id(void) const { return id; }
  bool Recording(void) const { return HasFlags(tfRecording); }
  bool Pending(void) const { return pending; }
  bool InVpsMargin(void) const { return inVpsMargin; }
  uint Flags(void) const { return flags; }
  const cChannel *Channel(void) const { return channel; }
  time_t Day(void) const { return day; }
  int WeekDays(void) const { return weekdays; }
  int Start(void) const { return start; }
  int Stop(void) const { return stop; }
  int Priority(void) const { return priority; }
  int Lifetime(void) const { return lifetime; }
  const char *File(void) const { return file; }
  time_t FirstDay(void) const { return weekdays ? day : 0; }
  const char *Aux(void) const { return aux; }
  const char *Remote(void) const { return remote; }
  bool Local(void) const { return !remote; } // convenience
  time_t Deferred(void) const { return deferred; }
  cString ToText(bool UseChannelID = false) const;
  cString ToDescr(void) const;
  const cEvent *Event(void) const { return event; }
  bool Parse(const char *s);
  bool Save(FILE *f);
  bool IsSingleEvent(void) const;
  static int GetMDay(time_t t);
  static int GetWDay(time_t t);
  bool DayMatches(time_t t) const;
  static time_t IncDay(time_t t, int Days);
  static time_t SetTime(time_t t, int SecondsFromMidnight);
  void SetFile(const char *File);
  bool Matches(time_t t = 0, bool Directly = false, int Margin = 0) const;
  eTimerMatch Matches(const cEvent *Event, int *Overlap = NULL) const;
  bool Expired(void) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  void SetId(int Id);
  bool SetEventFromSchedule(const cSchedules *Schedules);
  bool SetEvent(const cEvent *Event);
  void SetRecording(bool Recording);
  void SetPending(bool Pending);
  void SetInVpsMargin(bool InVpsMargin);
  void SetDay(time_t Day);
  void SetWeekDays(int WeekDays);
  void SetStart(int Start);
  void SetStop(int Stop);
  void SetPriority(int Priority);
  void SetLifetime(int Lifetime);
  void SetAux(const char *Aux);
  void SetRemote(const char *Remote);
  void SetDeferred(int Seconds);
  void SetFlags(uint Flags);
  void ClrFlags(uint Flags);
  void InvFlags(uint Flags);
  bool HasFlags(uint Flags) const;
  void Skip(void);
  void OnOff(void);
  cString PrintFirstDay(void) const;
  static int TimeToInt(int t);
  static bool ParseDay(const char *s, time_t &Day, int &WeekDays);
  static cString PrintDay(time_t Day, int WeekDays, bool SingleByteChars);
  };

class cTimers : public cConfig<cTimer> {
private:
  static cTimers timers;
  static int lastTimerId;
  time_t lastDeleteExpired;
public:
  cTimers(void);
  static const cTimers *GetTimersRead(cStateKey &StateKey, int TimeoutMs = 0);
      ///< Gets the list of timers for read access. If TimeoutMs is given,
      ///< it will wait that long to get a write lock before giving up.
      ///< Otherwise it will wait indefinitely. If no read lock can be
      ///< obtained within the given timeout, NULL will be returned.
      ///< The list is locked and a pointer to it is returned if the state
      ///< of the list is different than the state of the given StateKey.
      ///< If both states are equal, the list of timers has not been modified
      ///< since the last call with the same StateKey, and NULL will be
      ///< returned (and the list is not locked). After the returned list of
      ///< timers is no longer needed, the StateKey's Remove() function must
      ///< be called to release the list. The time between calling
      ///< cTimers::GetTimersRead() and StateKey.Remove() should be as short
      ///< as possible. After calling StateKey.Remove() the list returned from
      ///< this call must not be accessed any more. If you need to access the
      ///< timers again later, a new call to GetTimersRead() must be made.
      ///< A typical code sequence would look like this:
      ///< cStateKey StateKey;
      ///< if (const cTimers *Timers = cTimers::GetTimersRead(StateKey)) {
      ///<    // access the timers
      ///<    StateKey.Remove();
      ///<    }
  static cTimers *GetTimersWrite(cStateKey &StateKey, int TimeoutMs = 0);
      ///< Gets the list of timers for write access. If TimeoutMs is given,
      ///< it will wait that long to get a write lock before giving up.
      ///< Otherwise it will wait indefinitely. If no write lock can be
      ///< obtained within the given timeout, NULL will be returned.
      ///< If a write lock can be obtained, the list of timers will be
      ///< returned, regardless of the state values of the timers or the
      ///< given StateKey. After the returned list of timers is no longer
      ///< needed, the StateKey's Remove() function must be called to release
      ///< the list. The time between calling cTimers::GetTimersWrite() and
      ///< StateKey.Remove() should be as short as possible. After calling
      ///< StateKey.Remove() the list returned from this call must not be
      ///< accessed any more. If you need to access the timers again later,
      ///< a new call to GetTimersWrite() must be made. The call
      ///< to StateKey.Remove() will increment the state of the list of
      ///< timers and will copy the new state value to the StateKey. You can
      ///< suppress this by using 'false' as the parameter to the call, in
      ///< which case the state values are left untouched.
      ///< A typical code sequence would look like this:
      ///< cStateKey StateKey;
      ///< if (cTimers *Timers = cTimers::GetTimersWrite(StateKey)) {
      ///<    // access the timers
      ///<    StateKey.Remove();
      ///<    }
  static bool Load(const char *FileName);
  static int NewTimerId(void);
  const cTimer *GetById(int Id) const;
  cTimer *GetById(int Id) { return const_cast<cTimer *>(static_cast<const cTimers *>(this)->GetById(Id)); };
  const cTimer *GetTimer(const cTimer *Timer) const;
  cTimer *GetTimer(const cTimer *Timer);
  const cTimer *GetMatch(time_t t) const;
  cTimer *GetMatch(time_t t) { return const_cast<cTimer *>(static_cast<const cTimers *>(this)->GetMatch(t)); };
  const cTimer *GetMatch(const cEvent *Event, eTimerMatch *Match = NULL) const;
  cTimer *GetMatch(const cEvent *Event, eTimerMatch *Match = NULL) { return const_cast<cTimer *>(static_cast<const cTimers *>(this)->GetMatch(Event, Match)); }
  const cTimer *GetNextActiveTimer(void) const;
  const cTimer *UsesChannel(const cChannel *Channel) const;
  bool SetEvents(const cSchedules *Schedules);
  bool DeleteExpired(void);
  void Add(cTimer *Timer, cTimer *After = NULL);
  void Ins(cTimer *Timer, cTimer *Before = NULL);
  void Del(cTimer *Timer, bool DeleteObject = true);
  bool GetRemoteTimers(const char *ServerName = NULL);
      ///< Gets the timers from the given remote machine and adds them to this
      ///< list. If no ServerName is given, all timers from all known remote
      ///< machines will be fetched. This function calls DelRemoteTimers() with
      ///< the given ServerName first.
      ///< Returns true if any remote timers have been added or deleted
  bool DelRemoteTimers(const char *ServerName = NULL);
      ///< Deletes all timers of the given remote machine from this list (leaves
      ///< them untouched on the remote machine). If no ServerName is given, the
      ///< timers of all remote machines will be deleted from the list.
      ///< Returns true if any remote timers have been deleted.
  void TriggerRemoteTimerPoll(const char *ServerName = NULL);
      ///< Sends an SVDRP POLL command to the given remote machine.
      ///< If no ServerName is given, the POLL command will be sent to all
      ///< known remote machines.
  };

// Provide lock controlled access to the list:

DEF_LIST_LOCK(Timers);

// These macros provide a convenient way of locking the global timers list
// and making sure the lock is released as soon as the current scope is left
// (note that these macros wait forever to obtain the lock!):

#define LOCK_TIMERS_READ  USE_LIST_LOCK_READ(Timers)
#define LOCK_TIMERS_WRITE USE_LIST_LOCK_WRITE(Timers)

class cSortedTimers : public cVector<const cTimer *> {
public:
  cSortedTimers(const cTimers *Timers);
  };

#endif //__TIMERS_H
