#
#ident	"@(#)Makefile	1.1	96/10/08 SMI"
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#

GET_NETMASK=	get_netmask

PROGS =		$(GET_NETMASK)
	
GET_NETMASK_OBJS =	$(GET_NETMASK).o

GET_NETMASK_SRCS=	$(GET_NETMASK_OBJS:.o=.c)

CLOBBERFILES=	$(PROGS)

SRCMSGS=	

include ../Makefile.app

ROOTBIN=        $(ROOT)/sbin

OBJECTS=	$(GET_NETMASK_OBJS)

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

LDLIBS = -lsocket -lnsl -lintl -lrpcsvc

ROOTSBINPROGS= $(PROGS:%=$(ROOTSBIN)/%)

$(GET_NETMASK) :=	OBJS = $(GET_NETMASK_OBJS)

OWNER=		root
GROUP=		sys
FILEMODE=	755

.KEEP_STATE:

all:		$(PROGS) $(TXTS)

$(PROGS):	$$(OBJS)
	$(LINK.cc) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

install:	all $(ROOTSBINPROGS)

$(ROOTSBIN)/%: %
	$(INS.file)

install_h:

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

include ../Makefile.targ


