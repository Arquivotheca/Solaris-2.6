#
# uts/i86pc/logi/Makefile
# Copyright (c) 1991-1996, by Sun Microsystems, Inc.
# All Rights Reserved.
#
#ident	"@(#)Makefile	1.5	96/08/01 SMI"
#
#	This makefile drives the production of the logi driver 
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
MODULE		= logi
OBJECTS		= $(LOGI_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(LOGI_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_DRV_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

#
#	Overrides.
#
CLOBBERFILES	+= $(MODULE)

#
# lint pass one enforcement
#
CFLAGS += -v

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
