#
# uts/i86pc/esa/Makefile
# Copyright (c) 1993-1995 by Sun Microsystems, Inc.
# All Rights Reserved.
#
#pragma ident	"@(#)Makefile	1.7	95/06/30 SMI"
#
#	This makefile drives the production of the esa "drv"
#	kernel module.
#
#	i86pc implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= esa
OBJECTS		= $(ESA_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(ESA_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/i86pc/io/dktp/hba

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

DEBUG_FLGS      = -DESA_DEBUG
$(NOT_RELEASE_BUILD)DEBUG_DEFS	+= $(DEBUG_FLGS)
# EXTRA_OPTIONS and EXTRA_CFLAGS are the correct variables to which
# driver-specific cpp and compiler flags should be appended to the cc
# command (see ../../Makefile.uts and ../../../Makefile.master)
EXTRA_OPTIONS   +=
EXTRA_CFLAGS    +=
CLOBBERFILES	+= $(MODULE)

#
#	Default build targets.
#
.KEEP_STATE:

all:		$(MODULE)

lint:		$(MODULE).lint

install:  	$(MODULE) $(ROOTMODULE) $(ROOT_CONFFILE)

#
#	Include common targets.
#
include $(UTSBASE)/i86pc/Makefile.targ
