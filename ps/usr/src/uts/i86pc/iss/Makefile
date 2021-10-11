#
# uts/i86pc/iss/Makefile
# Copyright (c) 1995 by Sun Microsystems, Inc. All rights reserved.
#
#ident	"@(#)Makefile	1.5	96/11/21 SMI"
#
#       This makefile drives the production of the iss kernel
#       driver module.
#
#       iss implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..
SECRETBASE	= $(UTSBASE)/SunSoft_Confidential

#
#       Define the module and installation location.
#
MODULE          = iss
ROOTMODULE      = $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR     = $(SECRETBASE)/i86pc/io/dktp/hba/iss

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc
#
#	Default build targets.
#
.KEEP_STATE:

all:

lint:

install:  	$(MODULE) $(ROOTMODULE) $(ROOT_CONFFILE)

clean:

clobber:

#
#	Include common targets.
#
#include from $(UTSBASE)/i86pc/Makefile.targ:
$(ROOT_MOD_DIR): $(BASE_INS_DIR)

$(BASE_INS_DIR) $(ROOT_MOD_DIRS):
	-$(INS.dir)

$(ROOT_DRV_DIR)/%:     % $(ROOT_MOD_DIR) $(ROOT_DRV_DIR)
	$(INS.file)

$(ROOT_CONFFILE):	$(SRC_CONFILE)	$(ROOT_CONFFILE:%/$(CONFFILE)=%)
	$(INS.conffile)
