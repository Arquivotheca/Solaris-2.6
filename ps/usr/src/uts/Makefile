#
# uts/Makefile
#
#pragma ident  "@(#)Makefile 1.74     96/08/13 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# include global definitions
#
include $(SRC)/Makefile.master

#
# List of architectures to build as part of the standard build.
#
MACH:sh=    echo ${MACH:-`uname -p`}
ARCHITECTURE:sh = test $MACH = i386 && echo i86 || echo $MACH
i86_PLATFORMS = i86pc
ppc_PLATFORMS = prep
sparc_PLATFORMS =

ARCH_PLAT_TARGETS = $(ARCHITECTURE) $($(ARCHITECTURE)_PLATFORMS)

all		:=	TARGET= all
install		:=	TARGET= install
install_h	:=	TARGET= install_h
clean		:=	TARGET= clean
clobber		:=	TARGET= clobber
lint		:=	TARGET= lint
clean.lint	:=	TARGET= clean.lint
check		:=	TARGET= check

.KEEP_STATE:

all install: install_h $(ARCH_PLAT_TARGETS)

clean clobber lint clean.lint: $(ARCH_PLAT_TARGETS)

$(ARCH_PLAT_TARGETS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

#
# The following is the list of directories which contain Makefiles with
# targets to install header file. The machine independent headers are
# installed by invoking the Makefile in the directory containing the
# header files. Machine and architecture dependent headers are installed
# by invoking the main makefile for that architecture/machine which,
# in turn, is responsible for invoking the Makefiles which install headers.
# It is done this way so as not to assume that all of the header files in
# the architecture/machine dependent subdirectories are in completely
# isomorphic locations.
#
COMMON_HDRDIRS= common/sys
ARCH_HDRDIRS = $(ARCHITECTURE)/sys
sparc_PLATFORM_HDRDIRS =
ppc_PLATFORM_HDRDIRS =
i86_PLATFORM_HDRDIRS =  i86pc/sys

ARCH_PLAT_HDRDIRS = $(ARCH_HDRDIRS) $($(ARCHITECTURE)_PLATFORM_HDRDIRS)

HDRDIRS= $(COMMON_HDRDIRS) $(ARCH_PLAT_HDRDIRS)
install_h check: $(HDRDIRS) $(ARCH_PLAT_TARGETS)

$(HDRDIRS): FRC
	@if test -d $@ ; then cd $@; pwd; $(MAKE) $(TARGET) ; fi

#
# The cscope.out file is made in the current directory and spans all
# supported architectures.
# this target probably can't be tracked by Nse.
# Things to note:
#	1. We use relative names for cscope and tags.
#	2. We *don't* remove the old cscope.out file, because cscope is
#	   smart enough to only build what has changed.  It can be
#	   confused, however, if files are renamed or removed, so it may
#	   be necessary to manually remove cscope.out if a lot of
#	   reorganization has occured.
#
CSDIR	= .
i86_CSDIRS      = $($(ARCHITECTURE)_PLATFORMS) common
ppc_CSDIRS      = $($(ARCHITECTURE)_PLATFORMS) common
sparc_CSDIRS    = $($(ARCHITECTURE)_PLATFORMS) common
CSPATHS = $($(ARCHITECTURE)_CSDIRS:%=$(CSDIR)/%)
CSINCS	= $(CSPATHS:%=-I%)
CSCOPE	= cscope
CTAGS	= ctags

.PRECIOUS:	cscope.out

cscope.out: cscope.files FRC
	${CSCOPE} -b -f `pwd`/cscope.out

cscope.files: FRC
	@-$(RM) cscope.files
	echo "$(CSINCS)" > cscope.files
	find $(CSPATHS) -name SCCS -prune -o \
	    -type d -name '.del-*' -prune -o -type f \
	    \( -name '*.[csh]' -o -name 'Makefile*' -o -name '*.il' \) \
	    -print >> cscope.files
	@wc -l cscope.files

#
# Create a tags data base, similar to above.
# Since assembler files now contain C fragments for lint,
# the lint fragments will allow ctags to "work" on assembler.
# Things to note:
#	1. We put plain files before headers, and .c before .s (because
#	   ctags doesn't understand assembly comments).
#	2. We *don't* sort the output of find, as we want functions in
#	   architecture directories to take precedence over those in
#	   sun, and those over those in common.
#
tags: tags.list FRC
	${CTAGS} -wt `cat tags.list`

# THIS IS A KLUDGE
# We should really only list the files we use to build the kernel; this
# picks up things like mpsas directories which are not used in normal
# kernels.

tags.list: cscope.files
	@$(RM) tags.list
	grep '\.c$$' cscope.files | grep -v '/mpsas/' | grep -v '/sas/' \
		> tags.list
	grep '\.h$$' cscope.files | grep -v '/mpsas/' | grep -v '/sas/' \
		>> tags.list
	grep '\.s$$' cscope.files | grep -v '/mpsas/' | grep -v '/sas/' \
		>> tags.list

FRC:

