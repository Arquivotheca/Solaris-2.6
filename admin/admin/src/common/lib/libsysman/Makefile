#
#ident  "@(#)Makefile 1.12     96/09/09 SMI"
#
# Copyright (c) 1994 by Sun Microsystems, Inc.
# All rights reserved
#
# src/common/lib/libsysman/Makefile
#

LIBRARY   =	libsysman.a
VERS	  =	.1

MSGMASTER =	
MSG	  =	
TEXTDOMAIN =    $(MSG)
MSGPO	  =	$(TEXTDOMAIN).po
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1
MSGFILE =      libsysman

EXPHDRS   =	sysman_codes.h sysman_iface.h sysman_types.h

IMPLHDRS  =	sysman_impl.h

OBJECTS =	do_as_root.o \
		alias_iface.o alias_impl.o \
		group_iface.o group_impl.o \
		host_iface.o host_impl.o \
		jobsched_iface.o jobsched_impl.o \
		printer_iface.o printer_impl.o \
		serial_iface.o serial_impl.o \
		sw_iface.o sw_impl.o \
		user_iface.o user_impl.o \
		sysman_util.o


SRCS      =	$(OBJECTS:.o=.c)
SRCMSGS	=	$(MSG).c $(MSG).h

include ../Makefile.lib

CLEANFILES=	$(MSGSRCS)

CLOBBERFILES=	$(DYNLIB) $(DYNLIBLINK) *% $(MSGPO)

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =	$(ROOTINCADMIN)
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(CPPFLAGS.master)

CFLAGS +=	$(DEBUG_CFLAGS)

.KEEP_STATE:

all: 		static message_file

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all \
		$(ROOTADMINLIB) $(ROOTADMINLIBS)
#		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK)


message_file:
		$(GENMSG) -x -d $(MSGFILE).msg *.c *.h

# build xxx_message.o file for inclusion in links of individual methods
$(MSGOBJ):	$(SRCMSGS)
 
# build xxx_message.c and xxx_message.h files
#       also build the xxx_message.po file that will be used as input to the
#       localization process -- not sure where to put it or how to distribute it
$(SRCMSGS):	$(MSGMASTER)
		$(MKMSGS) -d $(TEXTDOMAIN) < $(MSGMASTER)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

include ../Makefile.targ

