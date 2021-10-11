#
# uts/i86pc/smceu/Makefile
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All Rights Reserved.
#
#pragma ident   "@(#)Makefile  1.1	95/07/18 SMI"
#
#	This makefile drives the production of the smceu driver 
#	kernel module.
#
#	i86pc architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= smceu
OBJECTS		= $(SMCEU_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(SMCEU_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/i86pc/io/smceu

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
EXTRA_CFLAGS	+= -DM_UNIX -DCODE_386 -DEBM -DUNIX=0x400
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
