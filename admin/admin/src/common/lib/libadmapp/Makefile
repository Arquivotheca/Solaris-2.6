#
#ident	"@(#)Makefile	1.31	94/10/25 SMI"
#
# Copyright (c) 1994 by Sun Microsystems, Inc.
# All rights reserved.
#
# classes/lib/libadmapp/Makefile
#

LIBRARY   =	libadmapp.a
VERS	  =	.2
PROG	  = 	libadmapp

MSGMASTER =	
MSG	  =	
TEXTDOMAIN =    $(MSG)
MSGPO	  =	$(TEXTDOMAIN).po
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1

EXPHDRS   =	adminhelp.h solstice.h UxXt.h

IMPLHDRS  =	

OBJECTS =	adminhelp.o solprep.o UxXt.o iconwin.o

SRCS      =	$(OBJECTS:.o=.c)
SRCMSGS	=	$(MSG).c $(MSG).h

include ../Makefile.lib

CLEANFILES=	$(MSGSRCS)

CLOBBERFILES=	$(DYNLIB) $(DYNLIBLINK) *% $(MSGPO)

# Where and how to install message file
LIBADMAPPMSGS	=	$(PROG:%=$(ROOTUSRLIBMSGS)/%.msg)
$(LIBADMAPPMSGS)	:=	FILEMODE = 0644

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =	$(ROOTINCADMIN) $(MOTIF_CFLAGS) $(X_CFLAGS)
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(CPPFLAGS.master)

CFLAGS +=	$(DEBUG_CFLAGS)

.KEEP_STATE:

all: 		static dynamic message_file

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all $(LIBADMAPPMSGS) \
		$(ROOTADMINLIB) $(ROOTADMINLIBS) \
		$(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)

message_file:
	$(GENMSG) -x -d libadmapp.msg *.c *.h

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
