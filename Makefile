#
# Makefile for the On Screen Menu of the Video Disk Recorder
#
# See the main source file 'osm.c' for copyright information and
# how to reach the author.
#
# $Id: Makefile 1.2 2000/03/05 13:51:57 kls Exp $

OBJS = config.o dvbapi.o interface.o menu.o osd.o recording.o remote.o tools.o osm.o

ifdef DEBUG_REMOTE
DEFINES += -DDEBUG_REMOTE
endif

ifdef DEBUG_OSD
DEFINES += -DDEBUG_OSD
endif

%.o: %.c
	g++ -g -O2 -Wall -c $(DEFINES) $<

all: osm

config.o   : config.c config.h dvbapi.h interface.h tools.h
dvbapi.o   : dvbapi.c config.h dvbapi.h interface.h tools.h
interface.o: interface.c config.h dvbapi.h interface.h remote.h tools.h
menu.o     : menu.c config.h dvbapi.h interface.h menu.h osd.h recording.h tools.h
osd.o      : osd.c config.h dvbapi.h interface.h osd.h tools.h
osm.o      : osm.c config.h dvbapi.h interface.h menu.h osd.h recording.h tools.h
recording.o: recording.c config.h dvbapi.h interface.h recording.h tools.h
remote.o   : remote.c remote.h tools.h
tools.o    : tools.c tools.h

osm: $(OBJS)
	g++ -g -O2 $(OBJS) -lncurses -o osm

clean:
	-rm $(OBJS) osm
