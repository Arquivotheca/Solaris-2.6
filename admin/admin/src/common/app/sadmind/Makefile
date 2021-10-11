#
#ident	"@(#)Makefile	1.30	96/01/12 SMI"	/* SunOS 5.0 */
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# cmd/sadmind/Makefile
#

PROG=		sadmind

EXPHDRS=

EXTRA_CC_LIBS=-nolib -lc -Bstatic -lC
OBJECTS=	sadmind.o

CLEANFILES=	$(OBJECTS)

include ../../cmd/Makefile.cmd

# Following variables define where to find header files
LOCINC=		.
RELINC=		../../lib/libadmapm ../../lib/libadmagt ../../lib/libadmcom \
		../../lib/libadmsec
GENINC=		$(ROOTINCADMIN)
CPPFLAGS=	$(LOCINC:%=-I%) $(RELINC:%=-I%) $(GENINC:%=-I%) \
		-D_SVR4_ -DADM_DEBUG $(CPPFLAGS.master)

CPPFLAGS += -D_REENTRANT

CFLAGS +=	$(DEBUG_CFLAGS)

LIBADMAGT=	-ladmagt
LIBADMAPM=	-ladmapm
LIBADMSEC=	-ladmsec
LIBADMCOM=	-ladmcom
LIBRPCSVC=	-lrpcsvc

$(PROG):= LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)

LDLIBS=		-L$(ROOTADMINLIB) $(LIBADMAGT) $(LIBADMSEC) $(LIBADMAPM) \
		$(LIBADMCOM) $(LIBRPCSVC) $(LDLIBS.cmd) -lintl -lthread

$(ROOTUSRSBINPROG) := FILEMODE=	0711
$(ROOTUSRSBINPROG) := OWNER=		root
$(ROOTUSRSBINPROG) := GROUP=		sys
$(ROOTUSRINCLEXP) := FILEMODE=	0444

.KEEP_STATE:

all: 		$(PROG)

$(PROG):	$(OBJECTS)
		$(LINK.cc) -o $@ $(OBJECTS) $(LDLIBS) $(EXTRA_CC_LIBS)
		$(POST_PROCESS)

install:	all $(ROOTUSRSBINPROG)

install_h:

lint:		lint_PROG

clean:
	$(RM) $(OBJECTS)

include ../Makefile.targ
