#
#ident  "@(#)Makefile	1.3	95/01/21 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved
#
# src/common/lib/libsysman/Makefile
#

LIBRARY   =	liblauncher.a
VERS	  =	.3

MSGMASTER =	
MSG	  =	
TEXTDOMAIN =    $(MSG)
MSGPO	  =	$(TEXTDOMAIN).po
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1

EXPHDRS   =	launcher_api.h

IMPLHDRS  =	launcher_p.h

OBJECTS =	sol_addapp.o sol_delapp.o sol_getapps.o sol_util.o


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

