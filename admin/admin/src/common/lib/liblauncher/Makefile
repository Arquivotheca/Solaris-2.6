#
#ident	"@(#)Makefile	1.9	95/01/21 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# all rights reserved.
#
# classes/lib/libadmutil/Makefile
#

LIBRARY   =	liblauncher-old.a
VERS	  =	.2

MSGMASTER =	
MSG	  =	
TEXTDOMAIN =    $(MSG)
MSGPO	  =	$(TEXTDOMAIN).po
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1

EXPHDRS   =	launcher.h launcher_p.h

IMPLHDRS  =	

OBJECTS =	appreg.o classreg.o invocation.o util.o

SRCS      =	$(OBJECTS:.o=.c)
SRCMSGS	=	$(MSG).c $(MSG).h

include ../Makefile.lib

CLEANFILES=	$(MSGSRCS)

CLOBBERFILES=	$(DYNLIB) $(DYNLIBLINK) \
		*% $(MSGPO)

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =	$(ROOTINCADMIN)
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(CPPFLAGS.master)

CFLAGS +=	$(DEBUG_CFLAGS)

.KEEP_STATE:

all: 		static

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

#install:	all \
#		$(ROOTADMINLIB) $(ROOTADMINLIBS) \
#		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK) \
#		$(ROOTCACHEOSLIBDIR) \
#		$(ROOTCACHEOSLIBDYNLIB) $(ROOTCACHEOSLIBDYNLIBLINK) \
#		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)
install:	all \
		$(ROOTADMINLIB) $(ROOTADMINLIBS)

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

