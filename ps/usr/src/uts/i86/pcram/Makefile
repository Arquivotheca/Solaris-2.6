#
# uts/i86/pcram/Makefile
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All Rights Reserved
#
#pragma ident	"@(#)Makefile	1.4	96/04/12 SMI"
#
#       This file makes the PCMCIA memory card driver for an i86 system
#
#	i86 architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= pcram
OBJECTS		= $(PCRAM_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(PCRAM_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR     = $(UTSBASE)/common/io/pcmcia

#
#	Include common rules.
#
include $(UTSBASE)/i86/Makefile.i86

DEBUG_FLGS      =
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

all:		$(MODULE) $(SRC_CONFILE)

lint:		$(MODULE).lint

install:  	$(MODULE) $(ROOTMODULE) $(ROOT_CONFFILE)

#
#	Include common targets.
#
include $(UTSBASE)/i86/Makefile.targ
