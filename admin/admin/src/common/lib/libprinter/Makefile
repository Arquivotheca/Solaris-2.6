#
#ident	"@(#)Makefile	1.8	95/02/20 SMI"
#
# Copyright (c) 1994 by Sun Microsystems, Inc.
#

LIBRARY   =	libprinter.a
VERS	  =	.2
MSGMASTER =	msg_master
MSG	=	printer_msgs
TEXTDOMAIN =	$(MSG)
MSGPO	=	$(TEXTDOMAIN).po

EXPHDRS   =	printer_iface.h # $(MSG).h

OBJECTS =	do_filters.o do_system.o \
		printer_add.o printer_delete.o printer_modify.o \
		printer_list.o printer_view.o \
		printer_utils.o \
		setup_network_access.o

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
