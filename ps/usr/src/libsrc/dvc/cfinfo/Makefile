#
# makefile for .cfinfo file installation
#
#ident "@(#)Makefile 1.34 96/06/06"
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All Rights Reserved.
#

include ../../../Makefile.master

TARGETS_DIFF =  at.cfinfo      	   \
	        kdmouse.cfinfo     \
		p9100.cfinfo	   \
		diamond.cfinfo	   \
		cirrus.cfinfo	   \
		monitor.cfinfo

TARGETS_ppc =   LOGI-S.cfinfo       \
	    	MOUSEMAN-S.cfinfo   \
	    	MS-S.cfinfo         \
	    	MSC-S.cfinfo        \
	    	Makefile	    \
	    	XDISPLAY.cfinfo     \
	    	abbreviations       \
	    	install.cfinfo      \
	    	master.cfinfo       \
	    	trackpoint.cfinfo   \
	    	s3-32.cfinfo        \
	    	s3-64.cfinfo        \
	    	wd.cfinfo

TARGETS_i386 =  8514.cfinfo         \
                LOGI-S.cfinfo       \
                MOUSEMAN-S.cfinfo   \
                MS-S.cfinfo         \
                MSC-S.cfinfo        \
                Makefile            \
                XDISPLAY.cfinfo     \
                abbreviations       \
                aha.cfinfo          \
                asy.cfinfo          \
                ast-notebook.cfinfo          \
                ati.cfinfo          \
                ata.cfinfo          \
                boca.cfinfo         \
                chips.cfinfo        \
		cirrus-notebook.cfinfo	   \
                compaq.cfinfo       \
                compaq-notebook.cfinfo       \
                dell.cfinfo         \
		dell-notebook.cfinfo\
                dec.cfinfo          \
                dpt.cfinfo          \
                eha.cfinfo          \
                el.cfinfo           \
                elsa.cfinfo         \
                elx.cfinfo          \
                everex.cfinfo       \
                hp.cfinfo           \
                ibm.cfinfo          \
                ibm-notebook.cfinfo \
                install.cfinfo      \
                intel.cfinfo        \
                intergraph.cfinfo   \
                logi.cfinfo         \
                lp.cfinfo           \
                mach32.cfinfo       \
                master.cfinfo       \
                matrox.cfinfo       \
                miro.cfinfo         \
                msm.cfinfo          \
                nanao928.cfinfo     \
                nanaoeizo.cfinfo    \
                nec.cfinfo          \
                number9.cfinfo      \
                oak.cfinfo          \
                orchid.cfinfo       \
                p9000.cfinfo        \
                sbpro.cfinfo        \
                smc.cfinfo          \
		spea.cfinfo	    \
                stb.cfinfo          \
		s3.cfinfo	    \
                ti.cfinfo           \
                tiqmouse.cfinfo     \
                tr.cfinfo           \
                trident.cfinfo      \
                toshiba.cfinfo      \
                tseng.cfinfo        \
                vga4.cfinfo         \
                vga4-excrt.cfinfo   \
                vga8-laptop.cfinfo  \
                vga8.cfinfo         \
                zenith.cfinfo


INSDIR=$(ROOT)/usr/lib/devconfig

$(TARGETS_DIFF): $$@.$(MACH)
		 rm -f $@
		 cp $@.$(MACH) $@
 
TARGETS = $(TARGETS_DIFF) $(TARGETS_$(MACH))
 
INSTALLABLES=$(TARGETS:%=$(INSDIR)/%)

# install rules
$(INSDIR)/%: %
	$(INS.file)

$(INSDIR):
	-$(INS.dir)

clobber:
	rm -f $(TARGETS_DIFF)

install: $(INSTALLABLES)
	
