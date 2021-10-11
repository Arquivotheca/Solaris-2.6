# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#ident	"@(#)Makefile	1.25	96/09/05 SMI"
#
#
# app/launcher/Makefile
#
PROG=		solstice

TEST=		solstice.test

SUBDIRS = 	help

SHFILES =

TXTS =

IMPLHDRS  =	action.h launcher.h util.h

OBJECTS=	apptable.o \
		action.o \
		config.o \
		exec_cb.o \
		fsb.o \
		layout.o \
		list.o \
		llibif.o \
		main.o \
		main_win.o \
		property.o \
		user.o \
		util.o

TEST_OBJ=	api.o

SRCS=		$(OBJECTS:.o=.c)
MSGFILES=	$(OBJECTS:.o=.msg)

include ../../Makefile.app

FILEMODE=     0555

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

SWINCLUDEDIR = -I$(ROOT)/usr/include -I$(ROOT)/usr/include/admin
SWLIBDIR = -L$(ROOT)/usr/lib -L$(ROOT)/usr/snadm/lib
SWLIBS =  -ladm

# where to find the X header files
CPPFLAGS=	-I. $(X_CFLAGS) $(MOTIF_CFLAGS) $(SWINCLUDEDIR) $(QAPINCPATH)

CFLAGS +=       $(DEBUG_CFLAGS) $(QAPFLAG)
CCFLAGS +=	$(DEBUG_CCFLAGS) $(QAPFLAG)

# For testing only
#RLINK_PATH	= -R$(ROOT)/usr/lib:$(ROOT)/usr/snadm/lib:/usr/openwin/lib:/usr/dt/lib
RLINK_PATH	= -R/usr/lib:/usr/snadm/lib:/usr/openwin/lib:/usr/dt/lib

LIBADM		= -Bstatic -ladmapp -Bdynamic -lnsl libXpm.a -llauncher
X_LIBS		= -lXm -lgen -lXt -lX11 -lDtWidget

# where to find the  X libraries
LDLIBS		+= $(LIBADM) $(SWLIBS) $(X_LIBS) $(QAPLIB)
LDLIBPATH	+= $(QAPLIBPATH) $(SWLIBDIR) $(X_LIBPATH) $(MOTIF_LIBPATH)

ADMINMSGS	=	$(PROG:%=$(ROOTUSRLIBMSGS)/%.msg)
$(ADMINMSGS)	:=	FILEMODE = 0644

# what gets installed, and where

all:=		TARGET=	all
install:=	TARGET= install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

.KEEP_STATE:

all:		libxpm $(PROG) message_file

install:	all rmprog $(ROOTPROG) $(ADMINMSGS) $(SUBDIRS)


$(PROG):	$(OBJECTS) 
		$(PURIFY) $(LINK.c) -o $@ $(OBJECTS) $(LDLIBPATH) $(LDLIBS) $(RLINK_PATH)
		$(POST_PROCESS)

$(TEST):	$(OBJECTS) $(TEST_OBJ)
		$(PURIFY) $(LINK.c) -o $@ $(OBJECTS) $(TEST_OBJ) $(LDLIBPATH) $(LDLIBS) $(RLINK_PATH)
		$(POST_PROCESS)

libxpm:	FRC
	cd libxpm/lib; make install

message_file:
	$(GENMSG) -x -d $(PROG).msg *.c *.h

install_h:

$(ROOTADMINETC):
	$(INS.dir)

rmprog:
	@ $(RM) $(ROOTPROG)

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)
	(cd libxpm; make clean)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

include ../../Makefile.targ
