#
#ident  "@(#)Makefile 1.58     96/08/13 SMI"
#
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
# Makefile for system source
#
# include global definitions
include Makefile.master
#
# the Targetdirs file is the AT&T target.dirs file in a makefile format.
# it defines TARGETDIRS and ROOTDIRS.
include Targetdirs

# Hardcoding of OPTFLAG (to -O) and including OPTFLAG in CFLAGS below
# overrides togglability of optimization using COPTFLAG variable in
# Makefile.master. In a case where a particular object/command must be built
# without optimization, the developer should be able to turn off optimization
# by setting COPTFLAG to null, but still use CFLAGS in the compile command
# line.
OPTFLAG =   $(COPTFLAG)
CPOPTFLAG = $(OPTFLAG)
MCSSTRING = $(RELEASE_CM)
# If += not used, overrides CPPFLAGS assignment in Makefile.master, which
# includes the ENVCPPFLAGS[1-4] assignments made in workspace env file.
CPPFLAGS += -DSVR4
# If += not used, overrides CFLAGS assignment in Makefile.master, which
# includes COPTFLAG - the toggle for optimization.
CFLAGS   += ${OPTFLAG} -Xa
LDFLAGS   = ${OPTFLAG}

COMMON_SUBDIRS = uts
i386_SUBDIRS = realmode util libsrc cmd
ppc_SUBDIRS = util libsrc cmd
sparc_SUBDIRS =

SUBDIRS= $($(MACH)_SUBDIRS) $(COMMON_SUBDIRS) pkgdefs

HDRSUBDIRS=     uts include cmd

all all_xmod :=		TARGET= all
install install_xmod :=	TARGET= install
install_h :=		TARGET= install_h
clean :=		TARGET= clean
clobber :=		TARGET= clobber
check :=		TARGET= check
_msg :=			TARGET= _msg

.KEEP_STATE:

# If RELEASE_BUILD set to null in shell environment, build source release
# archives
all: sgs $(SUBDIRS)
install: sgs $(SUBDIRS)

clean: $(SUBDIRS)

clobber: cms $(SUBDIRS)
	rm -fr $(INCDESTDIR) $(LIBDESTDIR)
	rm -fr $(ROOT)
	rm -fr $(PKGARCHIVE)

cms: $(RELEASE_BUILD) clobber_make_state

clobber_make_state:
	find . -name '.make.state' -exec rm -f {} \;

# for a complete build from scratch
crankturn: sgs uts
	@cd libsrc; pwd; $(MAKE) install
	@cd cmd; pwd; $(MAKE) all

#
# target for building a proto area for reference via the ROOT macro
#
protolibs: rootlibs

# Base subset of rootproto, excluding ucb libraries
#
rootlibs: sgs
	@cd libsrc; pwd; $(MAKE) install

# create target-variant symlinks
links:
	@cd uts; pwd; $(MAKE) links

$(SUBDIRS) : FRC
	@(cd $@; pwd; $(MAKE) 'OPTFLAG=${OPTFLAG}' \
		'CPOPTFLAG=${CPOPTFLAG}' "LINTERR=${LINTERR}" \
		'MCSSTRING=${MCSSTRING}' ${TARGET})

.PARALLEL:	sysheaders 

sgs: rootdirs .WAIT sysheaders .WAIT

#
rootdirs: $(ROOTDIRS)

$(ROOTDIRS):
	@$(INS.dir)

sysheaders: FRC
	@cd uts; pwd; $(MAKE) install_h

all_xmod install_xmod: FRC
	@cd uts/common/sys; pwd; $(MAKE) svvs_h

FRC:

