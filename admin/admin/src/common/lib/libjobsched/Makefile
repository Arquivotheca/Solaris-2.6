#
#ident  "@(#)Makefile	1.5	95/07/21 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved
#
# src/common/lib/libjobsched/Makefile
#

LIBRARY   =	libjobsched.a
VERS	  =	.1

MSGMASTER =	
MSG	  =	
TEXTDOMAIN =    $(MSG)
MSGPO	  =	$(TEXTDOMAIN).po
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1

EXPHDRS   =	jobsched_iface.h jobsched_codes.h jobsched_types.h

IMPLHDRS  =	

OBJECTS =	jobsched_add.o jobsched_delete.o jobsched_modify.o \
		jobsched_list.o


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

all: 		dynamic

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all \
		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK)
#		$(ROOTADMINLIB) $(ROOTADMINLIBS)

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

