#
# uts/common/sys/Makefile
#
#pragma ident  "@(#)Makefile 1.71     96/09/23 SMI"
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
# include global definitions
include ../../../Makefile.master

HDRS=

# "pc" header files, common to i386 and ppc
PCHDRS= \
asy.h		fd_debug.h	fdc.h		fdmedia.h \
lp.h		mse.h		vuidmice.h

# "cmdk" headers used on i386 and ppc
DKTPHDRS= \
ata.h		bbh.h		cdtypes.h	cm.h		cmdev.h \
cmdk.h		controller.h	dadev.h		dadk.h		dklb.h \
fctypes.h	flowctrl.h	gda.h		gen_vtoc.h \
hba.h		objmgr.h	quetypes.h	queue.h		scdk.h \
scdkwatch.h	sctarget.h	snlb.h		tgcd.h		tgcom.h \
tgdk.h		tgpassthru.h	tgtp.h

# "ws" header files, common to i386 and ppc
WSHDRS= \
8042.h

ROOTDIR=	$(ROOT)/usr/include/sys
ROOTPCDIR=	$(ROOTDIR)
ROOTDKTPDIR=	$(ROOTDIR)/dktp
ROOTWSDIR=	$(ROOTDIR)/ws
i386_ROOTDIRS=	$(ROOTDKTPDIR) $(ROOTWSDIR)
ppc_ROOTDIRS=	$(ROOTDKTPDIR) $(ROOTWSDIR)
sparc_ROOTDIRS=
ROOTDIRS=       $(ROOTDIR) $($(MACH)_ROOTDIRS)

ROOTPCHDRS= $(PCHDRS:%=$(ROOTPCDIR)/%)
ROOTDKTPHDRS= $(DKTPHDRS:%=$(ROOTDKTPDIR)/%)
ROOTWSHDRS= $(WSHDRS:%=$(ROOTWSDIR)/%)

i386_ROOTHDRS=	$(ROOTPCHDRS) $(ROOTDKTPHDRS) $(ROOTWSHDRS)
ppc_ROOTHDRS=	$(ROOTPCHDRS) $(ROOTDKTPHDRS) $(ROOTWSHDRS)
sparc_ROOTHDRS=

ROOTHDRS= $(HDRS:%=$(ROOTDIR)/%) $($(MACH)_ROOTHDRS)

# install rules
$(ROOTDIR)/%: %
	$(INS.file)

$(ROOTDIR)/dktp/%: dktp/%
	$(INS.file)

$(ROOTDIR)/ws/%: ws/%
	$(INS.file)

.KEEP_STATE:

install_h: $(ROOTDIRS) .WAIT $(ROOTHDRS)

$(ROOTDIRS):
	$(INS.dir)

FRC:
