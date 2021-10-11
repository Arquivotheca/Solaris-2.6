#
# uts/sparc/Makefile
#
#pragma ident  "@(#)Makefile 1.2     95/02/23 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
#	This makefile drives the production of all implementation architecture
#	independent modules for the SPARC processor. (For those unsure, this
#	means the module will run on all SPARC processor based machines
#	running SunOS.)

UTSBASE = ..

include Makefile.sparc

LINT_LIBS        = $(LINT_LIB) \
		   $(KMODS:%=$(LINT_LIB_DIR)/llib-l%.ln)

#
#
#
all		:=	TARGET= all
install		:=	TARGET= install
clean		:=	TARGET= clean
clobber		:=	TARGET= clobber
lint		:=	TARGET= lint
modlintlib	:=	TARGET= modlintlib
clean.lint	:=	TARGET= clean.lint
check		:=	TARGET= check

.KEEP_STATE:

.PARALLEL:      $(KMODS)

all install clean clobber: $(KMODS)

modlintlib clean.lint: $(KMODS)

$(KMODS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

install_h check:	FRC
	@if test -d sys ; then cd sys; pwd; $(MAKE) $(TARGET) ; fi

#
#	Full kernel lint target.
#
lint:	modlintlib
	@-$(ECHO) "\nFULL KERNEL: global crosschecks:"
	@$(LINT) $(LINTFLAGS) $(LINT_LIBS)

FRC:

