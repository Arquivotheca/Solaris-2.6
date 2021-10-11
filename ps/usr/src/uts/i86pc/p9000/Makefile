#
# uts/i86pc/p9000/Makefile
# Copyright (c) 1991-1995 by Sun Microsystems, Inc.
# All Rights Reserved.
#
#pragma ident  "@(#)Makefile 1.5     96/04/12 SMI"
#
#       This makefile drives the production of Weitek p9000 display driver
#	kernel module.
#
#       i86pc implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE          = p9000
OBJECTS         = $(P9000_OBJS:%=$(OBJS_DIR)/%)
LINTS           = $(P9000_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR     = $(UTSBASE)/i86pc/io/p9000

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc
 
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
include $(UTSBASE)/i86pc/Makefile.targ
