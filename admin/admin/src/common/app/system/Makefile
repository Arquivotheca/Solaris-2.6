#
# @(#)Makefile 1.22 94/09/16 SMI
#
# Copyright (c) 1992 by Sun Microsystems, Inc.
#

# Define the class name
CLASS	= 	system
CLASS_VERS = 	2.1
ACL	=	.acl
ACLLOCK =	$(ACL)lock
MSG	=	system_msgs
TEXTDOMAIN =	$(MSG)
MSGOBJ	=	pics/$(MSG).o
MSGLIB	=	lib$(MSG).so.1
MSGPO	=	$(TEXTDOMAIN).po

# Define the C-program method names as target variables (one per method)
ADMPIPE =			admpipe

HELP =		$(CLASS).info $(CLASS).hh

# Define the object files
OBJS = $(ADMPIPE).o

# Define the set of C-program methods to be made (list of method targets)
PROGS =		$(ADMPIPE)
UPROGS =	$(PROGS)

# Define the set of C-program source files used by this Makefile
SRCS =		$(OBJS:.o=.c)

# Define the set of files to be cleaned up by this Makefile
CLOBBERFILES =	$(PROGS) $(ACLLOCK) *%

# Define the exported headers for this class
EXPHDRS =	admpipe.h
IMPHDRS =

include ../../cmd/Makefile.cmd

# Following variables define where to find header files
LOCINC =	.
RELINC =	
GENINC =	$(ROOTINCADMIN)
CPPFLAGS =	$(LOCINC:%=-I%) $(RELINC:%=-I%) $(GENINC:%=-I%) \
		$(CPPFLAGS.master)

CFLAGS +=       $(DEBUG_CFLAGS)

# link time search for private libraries first in local build tree to get 
# "latest" version, then in normal location to get "previous" version
LIBADMAPM =	-L$(ROOTADMINLIB) -ladmapm -ladmagt -ladmsec -ladmcom -lrpcsvc \
		-lsocket -lnsl
LIBINTL =	-lintl

# conditional assignments
pics/%.o  :=	CPPFLAGS.master += -K PIC

# Definition of full path target definitions (based on Makefile.class)
ROOTADMINCLASSDIR23 =	$(ROOTADMINCLASS)/$(CLASS)
ROOTADMINCLASSDIR =	$(ROOTADMINCLASS)/$(CLASS).$(CLASS_VERS)
ROOTOPTADMINCLASSDIR =	$(ROOTOPTADMINCLASS)/$(CLASS).$(CLASS_VERS)
ROOTADMINCLASSPROGS =	$(PROGS:%=$(ROOTADMINCLASSDIR)/%)
ROOTADMINCLASSPROGS23 =	$(PROGS:%=$(ROOTADMINCLASSDIR23)/%)
ROOTOPTADMINCLASSPROGS =$(PROGS:%=$(ROOTOPTADMINCLASSDIR)/%)
ROOTADMINCLASSSCRIPTS =	$(SCRIPTS:%=$(ROOTADMINCLASSDIR)/%)
ROOTCLASSACL 	=	$(ACL:%=$(ROOTADMINCLASSDIR)/%)
ROOTCLASSACLLOCK =	$(ACLLOCK:%=$(ROOTADMINCLASSDIR)/%)
ROOTOPTCLASSACL	=	$(ACL:%=$(ROOTOPTADMINCLASSDIR)/%)
ROOTOPTCLASSACLLOCK =	$(ACLLOCK:%=$(ROOTOPTADMINCLASSDIR)/%)

# Following defines protections under which methods should be installed.
$(ROOTUSRINCLEXP) 	:=	FILEMODE = 644
$(ROOTADMINCLASSPROGS)	:=	FILEMODE = 0711
$(ROOTADMINCLASSPROGS23) :=	FILEMODE = 0711
$(ROOTADMINCLASSSCRIPTS) :=	FILEMODE = 0755
$(ROOTCLASSACL)		:=	FILEMODE = 0644
$(ROOTCLASSACLLOCK)	:=	FILEMODE = 0644

$(ROOTADMINCLASSDIR) :=	DIRMODE = 711
$(ROOTADMINCLASSLIBDIR)	:=	DIRMODE = 0711

# Unconditional addition of LD_RUN_PATH to link of all methods to force
# run time search for libraries to pick up "standard" version even if
# LD_LIBRARY_PATH isn't set
all				      := LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)
$(ROOTADMINCLASSDIR)/$(PROGS)   := LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)
$(ROOTADMINCLASSDIR23)/$(PROGS)   := LDFLAGS += -R$(ROOTADMINLIB:$(ROOT)%=%)
uall				      := LDFLAGS += -R$(ROOTOPTADMINLIB:$(ROOT)%=%)
$(ROOTOPTADMINCLASSDIR)/$(PROGS):= LDFLAGS += -R$(ROOTOPTADMINLIB:$(ROOT)%=%):.

LDLIBS +=	$(LIBS) $(LIBADMAPM) $(LIBINTL)

# No changes required below this line for adding new methods
.KEEP_STATE:

all: 		$(PROGS)

uall:		$(PROGS)

meth:		$(PROGS)

$(ACLLOCK):
		$(TOUCH) $@

$(PROGS):	$$(OBJS)
		$(PURIFY) $(LINK.cc) -o $@ $(OBJS) $(LDLIBS)
		$(POST_PROCESS)

$(OBJS):

install:	$(ROOTADMINCLASSDIR) $(ROOTOPTADMINCLASSDIR) \
		$(ROOTADMINCLASSDIR23) $(ROOTADMINCLASSPROGS23) \
		$(ROOTADMINCLASSPROGS) \
		$(ROOTOPTADMINCLASSPROGS) \
		$(ACLLOCK) \
		$(ROOTCLASSACL)	$(ROOTCLASSACLLOCK) \
		$(ROOTOPTCLASSACL) $(ROOTOPTCLASSACLLOCK)

install_h:	$(ROOTUSRINCLEXP) 

objs profs pics libp:
	-@mkdir $@ 2>/dev/null ; $(CHMOD) 777 $@

# build rule for "portable" source
objs/%.o profs/%.o pics/%.o: %.c
	$(PURIFY) $(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

$(ROOTADMINCLASSDIR)/%: %
	$(INS.file)

$(ROOTADMINCLASSDIR):
	$(INS.dir)

$(ROOTADMINCLASSDIR23):
	$(INS.dir)

$(ROOTADMINCLASSDIR23)/%: %
	$(INS.file)

$(ROOTOPTADMINCLASSDIR)/%: %
	$(INS.file)

$(ROOTOPTADMINCLASSDIR):
	$(INS.dir)

$(ROOTINCADMIN)/%: %
	$(INS.file)

lint:		lint_SRCS

clean:
	$(RM) $(OBJS)

include ../Makefile.targ
