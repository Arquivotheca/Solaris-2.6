#
# uts/i86/pcelx/Makefile
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#pragma ident   "@(#)Makefile	1.5     96/05/20 SMI"
#
#       This file makes the 3COM PCMCIA memory card driver for a x86 system
#
#	x86 architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= pcelx
OBJECTS		= $(PCELX_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(PCELX_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)

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

CFLAGS		+= -v

#
#	Default build targets.
#
.KEEP_STATE:

all:		$(MODULE)

lint:		$(MODULE).lint

install:  	$(MODULE) $(ROOTMODULE)

#
#	Include common targets.
#
include $(UTSBASE)/i86/Makefile.targ
