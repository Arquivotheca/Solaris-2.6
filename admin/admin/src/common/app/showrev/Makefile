#
#ident	"@(#)Makefile	1.24	95/10/31 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#

SHOWREV=	showrev

PROGS =		$(SHOWREV)
	
SHOWREV_OBJS =	$(SHOWREV).o

SHOWREV_SRCS=	$(SHOWREV_OBJS:.o=.c)

CLOBBERFILES=	$(PROGS)

SRCMSGS=	

include ../Makefile.app

ROOTBIN=        $(ROOT)/usr/bin

OBJECTS=	$(SHOWREV_OBJS)

TXTS=

LOCINC=		.
RELINC=
GENINC=		$(ROOTINCADMIN)

CPPFLAGS +=	$(LOCINC:%=-I%) $(RELINC:%=-I%) $(GENINC:%=-I%)

CFLAGS +=       $(DEBUG_CFLAGS)

LINTFLAGS=	-ux
NATIVECC=	$(CC)

# `Cause it blows up with $(TARGET_ARCH)
LINT.c=		$(LINT) $(LINTFLAGS) $(CPPFLAGS)

LIBS=		-ladmapm -ladmagt -ladmsec -ladmcom -ladmutil -lrpcsvc -lelf

$(PROGS):= LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)

LDLIBS +=	-L$(ROOTADMINLIB) $(LIBS) -lintl -lnsl -lsocket -lgen

ROOTBINPROGS= $(PROGS:%=$(ROOTBIN)/%)

$(SHOWREV) :=	OBJS = $(SHOWREV_OBJS)

OWNER=		root
GROUP=		sys
FILEMODE=	755

.KEEP_STATE:

all:		$(PROGS) $(TXTS)

$(PROGS):	$$(OBJS)
	$(LINK.cc) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

install:	all $(ROOTBINPROGS)

$(ROOTBIN)/%: %
	$(INS.file)

install_h:

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

include ../Makefile.targ


