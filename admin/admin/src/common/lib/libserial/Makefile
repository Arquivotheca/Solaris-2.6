#
#ident	"@(#)Makefile	1.6	95/01/13 SMI"
#
# Copyright (c) 1994 by Sun Microsystems, Inc.
#

LIBRARY   =	libserial.a
VERS	  =	.2
MSGMASTER =	msg_master
MSG	=	serial_msgs
TEXTDOMAIN =	$(MSG)
MSGPO	=	$(TEXTDOMAIN).po

EXPHDRS   =	serial_iface.h # $(MSG).h

OBJECTS =	pipe_execv.o \
		serial_add.o serial_delete.o serial_list.o \
		serial_globals.o serial_utils.o ttyutil.o

SRCS      =	$(OBJECTS:.o=.c)

MSGSRCS	=	$(MSG).c $(MSG).h

include ../Makefile.lib

CLEANFILES=	$(MSGSRCS)

CLOBBERFILES=	$(MSGPO) $(DYNLIB) $(DYNLIBLINK) *%

TXTS =

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =	$(ROOTINCADMIN)
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(CPPFLAGS.master)
# CFLAGS +=	$(DEBUG_CFLAGS) -DNIS_DEBUG
CFLAGS +=	$(DEBUG_CFLAGS)

SOFLAGS +=       -lkvm 
COSSOFLAGS +=    -lkvm

#Following variables set for install pathnames

.KEEP_STATE:

all: 		static dynamic $(TXTS)

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)


install:	all $(ROOTADMINLIB) $(ROOTADMINLIBS) \
		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)

# build xxx_message.c and xxx_message.h files
#	also build the xxx_message.po file that will be used as input to the
#	localization process -- not sure where to put it or how to distribute it
$(MSGSRCS):	$(MSGMASTER)
		$(MKMSGS) -d $(TEXTDOMAIN) < $(MSGMASTER)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

include ../Makefile.targ
