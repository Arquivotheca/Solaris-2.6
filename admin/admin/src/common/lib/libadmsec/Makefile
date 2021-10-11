#
#ident  "@(#)Makefile 1.0 90/11/12 SMI"
#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#
# lib/libadmsec/Makefile
#


LIBRARY   =	libadmsec.a
VERS	  =	.2
ADM_TEXT_DOMAIN = SYS_admfw
SVR4FLAG  =	-D_SVR4_
SYMFLAGS   =	-DADM_TEXT_DOMAIN=\"$(ADM_TEXT_DOMAIN)\"	\
		-DADM_DEBUG
PICFLAG   =	-K PIC

EXPHDRS   =	adm_auth.h

IMPLHDRS  =	adm_auth_impl.h

OBJECTS =	adm_auth_access.o	adm_auth_convert.o	\
		adm_auth_update.o	adm_auth_verify.o	\
		adm_auth_error.o	adm_auth_impl.o

SRCS      =	$(OBJECTS:.o=.c)

include ../Makefile.lib

CLOBBERFILES=	$(DYNLIB)
TXTS =

# Following variables define where to find header files
LOCAL    =	.
RELATED  =	../../lib/libadmapm	../../lib/libadmagt
GENERAL  =	
CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(SVR4FLAG) $(SYMFLAGS) $(CPPFLAGS.master)

CFLAGS +=       $(DEBUG_CFLAGS)

.KEEP_STATE:

all: 		static dynamic $(TXTS)

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

install:	all $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)

install_h:	$(ROOTUSRINCLEXP)

lint:		$(LINTLIB)

include ../Makefile.targ
