#
#ident	"@(#)Makefile	1.12	94/10/25 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# lib/libadmcom/Makefile
#

LIBRARY     =	libadmcom.a
VERS	    =	.2
NETMGT_TEXT_DOMAIN = SYS_admcom
SVR4FLAG    =	-D_SVR4_
SYMFLAGS    =   -DNETMGT_TEXT_DOMAIN=\"$(NETMGT_TEXT_DOMAIN)\"
PICFLAG	    =   -PIC
NOIDFLAG    =	-DNOID

EXPHDRS   =

OBJECTS =	action.o \
		activity.o \
		agent.o \
		arglist.o \
		auth.o \
		buffer.o \
		cache.o \
		callback.o \
		client.o \
		config.o \
		debug.o \
		decode.o \
		deferred.o \
		encode.o \
		errlist.o \
		error.o \
		event.o \
		fetch.o \
		generic.o \
		getarg.o \
		globals.o \
		header.o \
		ident.o \
		invoke.o \
		instance.o \
		kill.o \
		lock.o \
		manager.o \
		msg.o \
		msginfo.o \
		oid.o \
		options.o \
		performer.o \
		ping.o \
		putarg.o \
		queue.o \
		reap.o \
		register.o \
		rendez.o \
		report.o \
		request.o \
		resend.o \
		response.o \
		restart.o \
		security.o \
		send.o \
		server.o \
		set.o \
		setarg.o \
		shutdown.o \
		signal.o \
		startup.o \
		status.o \
		thresh.o \
		term.o \
		tli-callback.o \
		tli-server.o \
		trap.o \
		verify.o 


SRCS      =	$(OBJECTS:.o=.cc)

include ../Makefile.lib

LD=		CC
CLOBBERFILES=	$(DYNLIB)
TXTS =

# Following variables define where to find header files
LOCAL    =	.
RELATED  =
GENERAL  =

CPPFLAGS =	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%) \
		$(SVR4FLAG) $(SYMFLAGS)  $(NOIDFLAG) $(CPPFLAGS.master)

CFLAGS +=       $(DEBUG_CFLAGS)
CCFLAGS +=	$(DEBUG_CCFLAGS)

CCFLAGS +=	-D _REENTRANT
.KEEP_STATE:

#all: 		static dynamic $(TXTS)

all: 		dynamic $(TXTS)

static:		$(LIBS)

dynamic:	$(DYNLIB) $(DYNLIBLINK)

# $(ROOTADMINLIBS)
install:	all $(ROOTADMINLIBDYNLIB) $(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK)

install_h:	$(ROOTUSRINCLEXP)

lint:
	@echo "No lint for c++"

objs/$(ARCH)/%.o profs/$(ARCH)/%.o pics/$(ARCH)/%.o: %.cc
	$(PURIFY) $(COMPILE.cc) -o $@ $<
	$(POST_PROCESS_O)

include ../Makefile.targ
