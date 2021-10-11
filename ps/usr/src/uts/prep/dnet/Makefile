#
# uts/prep/dnet/Makefile
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident	"@(#)Makefile	1.3	95/06/23 SMI"
#
#	This makefile drives the production of the 'dnet' driver 
#	kernel module.
#
#	prep architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= dnet
OBJECTS		= $(DNET_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(DNET_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_DRV_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep

#
#	Overrides.
#
CLOBBERFILES	+= $(MODULE)
#
# Bug id 1211794. Temporarily turn off compiler optimization.
COPTFLAG= 

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
include $(UTSBASE)/prep/Makefile.targ
