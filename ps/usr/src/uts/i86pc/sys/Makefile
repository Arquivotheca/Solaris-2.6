#
# uts/i86pc/sys/Makefile
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile	1.22	96/07/17 SMI"
#
# include global definitions
include ../../../Makefile.master

HDRS= \
ansi.h		at_ansi.h	bt485reg.h	chanmux.h	char.h \
el.h		elx.h		emap.h \
evc.h		kb.h		kd.h		kdvm_cgi.h \
mouse.h		p9000reg.h	p9100reg.h	sbpro.h		smc.h \
smcboard.h	smchdw.h	vdc.h		vid.h		viperio.h \
viperreg.h	vt.h		vuidkd.h

DKTPHDRS= \
eha.h		mcis.h		aha.h		mscsi.h

DPTHDRS= \
dpt.h		dptsig.h	dptioctl.h

MLXHDRS= \
debug.h 	mlx.h		mlx_dac.h	mlx_dacioc.h	mlx_scsi.h \
mlxdefs.h	mlxops.h

ROOTDIR=	$(ROOT)/usr/include/sys
ROOTDKTPDIR=	$(ROOTDIR)/dktp
ROOTDPTDIR=	$(ROOTDIR)/dktp/dpt
ROOTMLXDIR=	$(ROOTDIR)/dktp/mlx
ROOTDIRS=       $(ROOTDIR) $(ROOTDKTPDIR) $(ROOTDPTDIR) $(ROOTMLXDIR) 

ROOTHDRS= $(HDRS:%=$(ROOTDIR)/%) \
	  $(DKTPHDRS:%=$(ROOTDKTPDIR)/%) \
	  $(DPTHDRS:%=$(ROOTDPTDIR)/%) \
	  $(MLXHDRS:%=$(ROOTMLXDIR)/%)

# install rules
$(ROOTDIR)/%: %
	$(INS.file)

$(ROOTDIR)/dktp/%: dktp/%
	$(INS.file)

$(ROOTDIR)/dktp/dtp/%: dktp/dtp/%
	$(INS.file)

$(ROOTDIR)/dktp/mlx/%: dktp/mlx/%
	$(INS.file)

.KEEP_STATE:

install_h: $(ROOTDIRS) .WAIT $(ROOTHDRS)

$(ROOTDIRS):
	$(INS.dir)
