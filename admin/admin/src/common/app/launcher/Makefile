#
#ident	"@(#)Makefile	1.32	96/08/28 SMI"
#
# Copyright (c) 1993 by Sun Microsystems, Inc.
# All rights reserved.
#
# app/launcher/Makefile
#
PROG =		solstice-old

SHFILES =

TXTS =

OBJECTS=	dyn_array.o main.o ui_layout.o

SRCS=		$(OBJECTS:.o=.c)

SUBDIRS=

include ../Makefile.app

# QA partner 1.1 support
$(QAPCH)QAPFLAG	=	-DPARTNER
$(QAPCH)QAPINCPATH	=	$(QAPINCPATH11)
$(QAPCH)QAPLIBPATH	=	$(QAPLIBPATH11)
$(QAPCH)QAPLIB		=	-lpartner

# QA partner 2.0 support
#$(QAPCH)QAPFLAG		=	-DPARTNER2
#$(QAPCH)QAPINCPATH	=	$(QAPINCPATH20)
#$(QAPCH)QAPLIBPATH	=	$(QAPLIBPATH20)
#$(QAPCH)QAPLIB		=	-lpartner

# where to find the header files
CPPFLAGS=	$(CPPFLAGS.master) $(X_CFLAGS) $(MOTIF_CFLAGS) $(SNAGINC) $(QAPINCPATH)

CFLAGS +=       $(DEBUG_CFLAGS) $(QAPFLAG)
CCFLAGS +=      $(DEBUG_CCFLAGS) $(QAPFLAG)

# where to find the libraries
LIBSYS =	-lsocket -lgen -lintl -lm

X_LIBS		= -l$(MOTIFLIB_NAME) -lXt -lX11

LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)

LDLIBS +=	-L$(ROOTADMINLIB) -llauncher-old $(X_LIBS) $(LIBSYS) -ladmapp \
		$(QAPLIB)

CONFIG			=	launcher.config
APPREG			=	appreg.db
CLASSREG		=	classreg.db
APPDEF			=	Admin
ICON			=	launcher.icon
CATLOG			=	launcher.cat

# what gets installed, and where
POLICY_MK_TARGET        =       policy_mk_target
POLICY_TBL_TEMPLATE     =       policy.defaults
ROOTADMINETCMKTARG      =       $(POLICY_MK_TARGET:%=$(ROOTADMINETC)/%)
ROOTADMINETCTEMPL       =       $(POLICY_TBL_TEMPLATE:%=$(ROOTADMINETC)/%)
$(ROOTADMINETCMKTARG)   :=      FILEMODE = 0644
$(ROOTADMINETCTEMPL)    :=      FILEMODE = 0644

# Definition of full path target definitions (based on Makefile.class)
ROOTADMINETCCONFIG 	=	$(CONFIG:%=$(ROOTADMINETC)/%)
ROOTADMINETCAPPREG 	=	$(APPREG:%=$(ROOTADMINETC)/%)
ROOTADMINETCCLASSREG 	=	$(CLASSREG:%=$(ROOTADMINETC)/%)
ROOTAPPDEFAULTSADMIN 	=	$(APPDEF:%=$(ROOTAPPDEFAULTS)/%)
ROOTADMINETCICON 	=	$(ICON:%=$(ROOTADMINETC)/%)
ROOTADMINETCCATLOG 	=	$(CATLOG:%=$(ROOTADMINETC)/%)

# Following defines protections under which methods should be installed.
$(ROOTADMINETCCONFIG)	:=	FILEMODE = 0644
$(ROOTADMINETCAPPREG)	:=	FILEMODE = 0644
$(ROOTADMINETCCLASSREG)	:=	FILEMODE = 0644
$(ROOTAPPDEFAULTSADMIN)	:=	FILEMODE = 0644
$(ROOTADMINETCICON)	:=	FILEMODE = 0644
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
		$(X_LIBPATH) $(MOTIF_LIBPATH) $(QAPLIBPATH) $(SNAGPATH) $(LDLIBS) $(RLINK_PATH)
		$(POST_PROCESS)

install:	all rmprog $(ROOTADMINETC) $(ROOTADMINETCCONFIG) \
		$(ROOTADMINETCAPPREG) $(ROOTADMINETCCLASSREG) \
		$(ROOTADMINETCICON) $(ROOTAPPDEFAULTS) $(ROOTAPPDEFAULTSADMIN) \
		$(ROOTADMINETCMKTARG) $(ROOTADMINETCTEMPL) \
		$(ROOTPROG) $(SUBDIRS)


gencat:	
	$(GENCAT) launcher.cat ../*/*.msg

# genmsg has a problem to take Japanese in -m prefix and *.c 
message_file:
	$(GENMSG) -d strings.msg adminhelp.c main.c ui_layout.c

$(ROOTADMINETC):
	$(INS.dir)

$(ROOTAPPDEFAULTS):
	$(INS.dir)

rmprog:
	@ $(RM) $(ROOTPROG) $(ROOTADMINETCCONFIG) \
	$(ROOTADMINETCAPPREG) $(ROOTADMINETCCLASSREG)

install_h:

install_config:

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

include ../Makefile.targ
