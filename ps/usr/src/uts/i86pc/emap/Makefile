#
# uts/i86pc/emap/Makefile
# Copyright (c) 1993-1994, Sun Microsystems, Inc.
#
#ident "@(#)Makefile	1.3	94/06/01 SMI"
#
# This makefile drives the production of the emap STREAMS kernel module.
#
# i86pc implementation architecture dependent
#

UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= emap
OBJECTS		= $(EMAP_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(EMAP_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_STRMOD_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

#
#	Overrides.
#
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
include $(UTSBASE)/i86pc/Makefile.targ
