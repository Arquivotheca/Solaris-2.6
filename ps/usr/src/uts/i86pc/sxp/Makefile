#
# uts/i86pc/sxp/Makefile
# Copyright (c) 1996 by Sun Microsystems, Inc.
#
#pragma ident	"@(#)Makefile	1.1	96/02/21 SMI"
#
#       This makefile drives the installation of the Rockwell "sxp"
#	kernel modules.
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULES         = rns_smt sxp sys_core
CONF_SRCDIR     = $(UTSBASE)/i86pc/io/sxp
CONFFILES       = $(CONF_SRCDIR)/rns_smt.conf $(CONF_SRCDIR)/sxp.conf

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc
#
#	Default build targets.
#
.KEEP_STATE:

all:            $(MODULES)

install:        $(MODULES) $(CONFFILES)
		-@for i in $(MODULES) ; do \
		    CF="" ; \
		    if `test -f $(CONF_SRCDIR)/$$i.conf` ; then \
			CF=$(ROOT_DRV_DIR)/$$i.conf ; \
		    fi ; \
		    $(MAKE) MODULE=$$i ROOTMODULE=$(ROOT_DRV_DIR)/$$i $(ROOT_DRV_DIR)/$$i $$CF; \
		done

clean:

clobber:

#
#	Include common targets.
#
# Following included from $(UTSBASE)/i86pc/Makefile.targ:

$(ROOT_MOD_DIR): $(BASE_INS_DIR)

$(BASE_INS_DIR) $(ROOT_MOD_DIRS):
	-$(INS.dir)

$(ROOT_DRV_DIR)/%:     % $(ROOT_MOD_DIR) $(ROOT_DRV_DIR)
	$(INS.file)

$(ROOT_CONFFILE):	$(SRC_CONFFILE)	$(ROOT_CONFFILE:%/$(CONFFILE)=%)
	$(INS.conffile)
