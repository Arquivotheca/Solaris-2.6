#
#ident	"@(#)Makefile	1.2	95/01/21 SMI"

#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# app/launcher/soladdapp/Makefile
#
PROG =		soladdapp

SHFILES =

TXTS =

OBJECTS=	soladdapp.o

SRCS=		$(OBJECTS:.o=.c)

SUBDIRS=

include ../../Makefile.app

# QA partner 1.1 support
#$(QAPCH)QAPFLAG	=	-DPARTNER
#$(QAPCH)QAPINCPATH	=	$(QAPINCPATH11)
#$(QAPCH)QAPLIBPATH	=	$(QAPLIBPATH11)
#$(QAPCH)QAPLIB		=	-lpartner

# QA partner 2.0 support
$(QAPCH)QAPFLAG		=	-DPARTNER2
$(QAPCH)QAPINCPATH	=	$(QAPINCPATH20)
$(QAPCH)QAPLIBPATH	=	$(QAPLIBPATH20)
$(QAPCH)QAPLIB		=	-lpartner -lsocket

# where to find the header files
CPPFLAGS=	$(CPPFLAGS.master) $(SNAGINC) $(QAPINCPATH)

CFLAGS +=       $(DEBUG_CFLAGS) $(QAPFLAG)
CCFLAGS +=      $(DEBUG_CCFLAGS) $(QAPFLAG)

# where to find the libraries

LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)

LDLIBS +=	-L$(ROOTADMINLIB) -llauncher $(QAPLIB)

CATLOG			=	soladdapp.cat

# Definition of full path target definitions (based on Makefile.class)
ROOTADMINETCCATLOG 	=	$(CATLOG:%=$(ROOTADMINETC)/%)

# Following defines protections under which methods should be installed.
$(ROOTADMINBINPROG)	:=	FILEMODE = 0555
$(ROOTADMINETCCATLOG)	:=	FILEMODE = 0644

# what gets installed, and where

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

.KEEP_STATE:

all:		$(PROG)

$(PROG):	$(OBJECTS)
		$(PURIFY) $(LINK.c) -o $@ $(OBJECTS) \
		$(QAPLIBPATH) $(SNAGPATH) $(LDLIBS) $(RLINK_PATH)
		$(POST_PROCESS)

install:	all rmprog $(ROOTADMINBINPROG)


gencat:	
	$(GENCAT) launcher.cat ../*/*.msg

# genmsg has a problem to take Japanese in -m prefix and *.c 
message_file:
	$(GENMSG) -d strings.msg soladdapp.c

$(ROOTADMINETC):
	$(INS.dir)

rmprog:
	@ $(RM) $(ROOTADMINBINPROG)

install_h:

install_config:

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

include ../../Makefile.targ
