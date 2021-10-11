#
#ident	"@(#)Makefile	1.27	95/10/31 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# cmd/apm/Makefile
#

APM=		apm
PROG =		$(APM)
MSGMASTER =	apm.msgs
MSG =		apm_msgs
MSG_DOMAIN =	SUNW_ADM_APM
TXTS =
SYMFLAGS =	-DASH_TEXT_DOMAIN=\"$(MSG_DOMAIN)\"
SVR4FLAG =	-D_SVR4_

EXPHDRS  =
IMPLHDRS =	apm_impl.h

APMOBJ =	$(APM).o

MSGPO	=	$(MSG_DOMAIN).po
MSGOBJ	=	$(MSG).o
SRCMSGS	=	$(MSG).c $(MSG).h $(MSGPO)

OBJECTS =	$(MSGOBJ) $(APMOBJ)

SRCS =		$(APMOBJ:.o=.c)

NATIVECC =	$(CC)

include ../../cmd/Makefile.cmd

CLOBBERFILES =	$(OBJECTS) $(SRCMSGS)

# Following variables define where to find header files
LOCAL    =	.
RELATED	=	../../lib/libadmapm ../../lib/libadmagt ../../lib/libadmcom \
		../../lib/libadmsec
GENERAL  =	$(ROOTINCADMIN)

CPPFLAGS +=	$(LOCAL:%=-I%) $(RELATED:%=-I%) $(GENERAL:%=-I%)	\
		$(SVR4FLAG) $(SYMFLAGS)

CFLAGS +=       $(DEBUG_CFLAGS)

LIBADMAPM   =	-ladmapm
LIBADMSEC   =	-ladmsec
LIBADMCOM   =	-ladmcom
LIBADMAGT   =	-ladmagt
LIBRPCSVC   =	-lrpcsvc

$(PROG):= LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)

LDLIBS += -L$(ROOTADMINLIB)  $(LIBADMAPM) $(LIBADMAGT) $(LIBADMSEC) \
	$(LIBADMCOM) $(LIBRPCSVC) -lintl

$(APM) :=	OBJS = $(APMOBJ) $(MSGOBJ)

#Following variable defines protections under which commands should be installed.
FILEMODE =	2755

#Target definitions
ROOTBIN     =	$(ROOT)/usr/bin
ROOTETC     =	$(ROOT)/usr/etc
ROOTBINPROG   =	$(PROG:%=$(ROOTBIN)/%)
ROOTETCTXTS   =	$(TXTS:%=$(ROOTETC)/%)

.KEEP_STATE:

$(MSGOBJ):	$(SRCMSGS)
		$(COMPILE.c) $(MSG).c

$(SRCMSGS):	$(MSGMASTER)
		$(MKMSGS) -d $(MSG_DOMAIN) < $(MSGMASTER)

all: 		$(PROG) $(TXTS)

$(APMOBJ):	$(SRCS) $(SRCMSGS)
		$(COMPILE.c) -o $@ $(SRCS)

$(PROG):	$$(OBJS)
		$(LINK.cc) -o $@ $(OBJS) $(LDLIBS)
		$(POST_PROCESS)

install:	all $(ROOTBINPROG) $(ROOTETCTXTS) $(INSTMSGS)

install_h:

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

include ../Makefile.targ
