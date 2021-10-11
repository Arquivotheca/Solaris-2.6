#
# uts/i86/Makefile
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All Rights Reserved
#
#pragma ident  "@(#)Makefile 1.7     95/06/07 SMI"
#
#	This makefile drives the production of all implementation architecture
#	independent modules for the Intel x86 processor.

UTSBASE = ..

include $(UTSBASE)/i86/Makefile.i86

all		:=	TARGET= all
install		:=	TARGET= install
clean		:=	TARGET= clean
clobber		:=	TARGET= clobber
lint		:=	TARGET= lint
lintlib		:=	TARGET= lintlib
clean.lint	:=	TARGET= clean.lint

.KEEP_STATE:

all install clean clobber lint lintlib clean.lint: $(KMODS)

$(KMODS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

install_h: FRC
	@cd sys; pwd; $(MAKE) install_h

FRC:
