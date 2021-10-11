#
# uts/i86pc/io/pcscsi/Makefile
#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#pragma ident	"@(#)Makefile	1.2	95/04/28 SMI"
#
#	This makefile drives the production of the pcscsi "drv"
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
MODULE		= pcscsi
OBJECTS		= $(PCSCSI_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(PCSCSI_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/i86pc/io/dktp/hba/pcscsi

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

#
#	Overrides.
#
CLOBBERFILES	+= $(MODULE)
#	DBG and MSG are exclusively for the AMD core code debug facility
DEBUG_FLGS	= -DPCSCSI_DEBUG -DDBG -DMSG
DEBUG_DEFS	+= $(DEBUG_FLGS)

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
