#
# uts/i86pc/Makefile
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
#ident @(#)Makefile	1.9	95/05/25 SMI
#
#	This makefile drives the production of all implementation architecture
#	dependent modules for the i86pc architecture.
#

UTSBASE = ..

include Makefile.i86pc

#
#	The following are i86 specific (rather than i86pc) specific modules
#	which are required for the i86pc kernel to completely lint. They are
#	not involved in the build in any other way. In order to minimize
#	build time, it is assumed that they are up to date.
#
I86_LIB_DIR	 = $(UTSBASE)/i86/lint-libs

I86_LINTS	 =

LINT_LIBS	 = $(LINT_LIB) \
		   $(KMODS:%=$(LINT_LIB_DIR)/llib-l%.ln) \
		   $(I86_LINTS:%=$(I86_LIB_DIR)/llib-l%.ln)

all		:=	TARGET= all
install		:=	TARGET= install
clean		:=	TARGET= clean
clobber		:=	TARGET= clobber
lint		:=	TARGET= lint
lintlib		:=	TARGET= lintlib
modlintlib	:=	TARGET= modlintlib
clean.lint	:=	TARGET= clean.lint

.KEEP_STATE:

all install clean clobber clean.lint: $(KMODS)

lintlib:

modlintlib:     $(KMODS)

.PARALLEL: $(KMODS)

$(KMODS):       FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

install_h:	FRC
	@cd sys; pwd; $(MAKE) install_h

#
#	Full kernel lint target.
#
lint:	lintlib modlintlib $(I86_LINTS)
	@-$(ECHO) "\nFULL KERNEL: global crosschecks:"
	@$(LINT) $(LINTFLAGS) $(LINT_LIBS)

$(I86_LINTS):
	@cd $(I86_LIB_DIR)/$@; pwd; make lintlib

#
# The cscope.out file is made in the current directory and spans only
# its architecture.
# Things to note:
#	1. We use relative names for cscope and tags.
#	2. We *don't* remove the old cscope.out file, because cscope is
#	   smart enough to only build what has changed.  It can be
#	   confused, however, if files are renamed or removed, so it may
#	   be necessary to manually remove cscope.out if a lot of
#	   reorganization has occured.
#
CSDIRS	= $(SRC)/uts/i86pc $(SRC)/uts/i86 $(SRC)/uts/common
CSINCS	= $(CSDIRS:%=-I%)
CSCOPE	= cscope
CTAGS	= ctags

.PRECIOUS: cscope.out

cscope.out: cscope.files FRC
	${CSCOPE} -b -f `pwd`/cscope.out

cscope.files: FRC
	@-$(RM) cscope.files
	echo "$(CSINCS)" > cscope.files
	find $(CSDIRS) -name SCCS -prune -o -type f \
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

