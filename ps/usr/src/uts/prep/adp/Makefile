#
# uts/prep/adp/Makefile
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#pragma ident	"@(#)Makefile	1.2	96/01/26 SMI"
#
#       This makefile drives the production of the adp "drv"
#       kernel module.
#
#	prep implementation architecture dependent
#

#
#       Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE = ../..

#
#       Define the module and object file sets.
#
MODULE          = adp
OBJECTS         = $(ADP_OBJS:%=$(OBJS_DIR)/%)
LINTS           = $(ADP_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE      = $(ROOT_PSM_DRV_DIR)/$(MODULE)
CONF_SRCDIR     = $(UTSBASE)/common/io/dktp/hba/adp
 
#
#       Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep

DEBUG_FLGS      = -DADP_DEBUG
$(NOT_RELEASE_BUILD)DEBUG_DEFS  += $(DEBUG_FLGS)
# EXTRA_OPTIONS and EXTRA_CFLAGS are the correct variables to which
# driver-specific cpp and compiler flags should be appended to the cc
# command (see ../../Makefile.uts and ../../../Makefile.master)
EXTRA_OPTIONS   +=
EXTRA_CFLAGS    +=
CLOBBERFILES    += $(MODULE)

#
#       Default build targets.
#
.KEEP_STATE:

all:            $(MODULE)

lint:           $(MODULE).lint

install:        $(MODULE) $(ROOTMODULE) $(ROOT_CONFFILE)

#
#       Include common targets.
#
include $(UTSBASE)/prep/Makefile.targ

