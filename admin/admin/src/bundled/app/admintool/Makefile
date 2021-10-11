# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#pragma ident "@(#)Makefile	1.40 96/10/11 Sun Microsystems"
#
#
# app/host/Makefile
#
PROG=		admintool

SUBDIRS=	adminhelp

SHFILES =

TXTS =

OBJECTS=	add_group.o		\
		add_host.o		\
		add_printer.o		\
		add_remote.o		\
		add_sw.o		\
		add_user.o		\
		bar.o			\
		basedir.o		\
		customize_sw.o		\
		fud.o			\
		locale.o		\
		main.o			\
		main_win.o		\
  		meter.o			\
		modify_group.o		\
		modify_host.o		\
		modify_printer.o	\
		modify_serial.o		\
		modify_sw.o		\
		modify_user.o		\
		module.o		\
		password.o		\
		pkg_admin.o		\
		source_media_dialog.o	\
		swlibif.o		\
		view_sw.o		\
		util.o

SRCS=		$(OBJECTS:.o=.c)

include ../Makefile.cmd

RESFILES	= 	Admin
RESOURCEFILES	=	$(RESFILES:%=$(ROOTRESD)/%)
ADMINMSGS	=	$(PROG:%=$(ROOTUSRLIBMSGS)/%.msg)
$(ADMINMSGS)	:=	FILEMODE = 0644

# QA partner 1.1 support
#$(QAPCH)QAPFLAG	=	-DPARTNER
#$(QAPCH)QAPINCPATH	=	$(QAPINCPATH11)
#$(QAPCH)QAPLIBPATH	=	$(QAPLIBPATH11)
#$(QAPCH)QAPLIB		=	-lpartner

# QA partner 2.0 support
$(QAPCH)QAPFLAG		=	-DPARTNER2
$(QAPCH)QAPINCPATH	=	$(QAPINCPATH20)
$(QAPCH)QAPLIBPATH	=	$(QAPLIBPATH20)
$(QAPCH)QAPLIB		=	-lpartner

SWINCLUDEDIR = -I$(ROOT)/usr/include -I$(ROOT)/usr/include/admin
#SWLIBDIR = -L$(ROOT)/usr/lib -L$(ROOT)/usr/snadm/lib -L/usr/ccs/lib
SWLIBDIR = -L$(PROTO)/usr/lib -L$(PROTO)/lib -L$(ROOT)/usr/lib -L$(ROOT)/usr/snadm/lib
SWLIBS = -lspmisvc -lspmisoft -lspmistore -lspmicommon 

# where to find the X header files
CPPFLAGS=	-DMETER $(X_CFLAGS) $(MOTIF_CFLAGS) $(SWINCLUDEDIR) -I. $(QAPINCPATH)

CFLAGS +=       $(DEBUG_CFLAGS) $(QAPFLAG) 
CCFLAGS +=	$(DEBUG_CCFLAGS) $(QAPFLAG)

# where and how to install icon
ICON		=	admintool.xpm
ROOTETCICON 	=	$(ICON:%=$(ROOTADMINETC)/%)
$(ROOTETCICON)	:=	FILEMODE = 0644

# uncomment for testing
# NEVER PUTBACK THIS MAKEFILE WITH THIS LINE UNCOMMENTED, IT WILL BREAK
# THIS APP IN THE DELIVERY PACKAGES.
#RLINK_PATH	= -R$(ROOT)/usr/lib:$(ROOT)/usr/snadm/lib:/usr/openwin/lib:/usr/dt/lib

LIBADM		= -lsysman -lprinter -lserial -ladmldb -ladmutil -ladmapp \
		  -lsocket -lnsl -ldevinfo -ldl -ladm
X_LIBS		= -lXm -lgen -lXt -lX11

# where to find the  X libraries
LDLIBS		+= $(SWLIBS) $(LIBADM) $(X_LIBS) $(QAPLIB)
LDLIBPATH	+= $(QAPLIBPATH) $(SWLIBDIR) $(X_LIBPATH) $(MOTIF_LIBPATH)

# what gets installed, and where

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

.KEEP_STATE:

all:		$(PROG) message_file

install:	all $(ROOTUSRBINPROG)  $(RESOURCEFILES) $(ADMINMSGS) \
		$(ROOTETCICON) $(SUBDIRS)


$(PROG):	$(OBJECTS)
		$(PURIFY) $(LINK.c) -o $@ $(OBJECTS) $(LDLIBPATH) $(LDLIBS) $(RLINK_PATH)
		$(POST_PROCESS)

$(ROOTADMINETC):
	$(INS.dir)

rmprog:
	@ $(RM) $(ROOTADMINBINPROG)

install_h:

message_file:
	$(GENMSG) -x -d $(PROG).msg *.c *.h

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

clobber: clean
	$(RM) $(PROG) $(CLOBBERFILES)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

include ../Makefile.targ
