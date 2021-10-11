#
# uts/i86pc/mscsi/Makefile
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
#pragma	ident	"@(#)Makefile	1.3	96/05/27 SMI"
#
#	This makefile drives the production of the mscsi
#	bus nexus driver kernel module.
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
MODULE		= mscsi
OBJECTS		= $(MSCSI_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(MSCSI_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/i86pc/io

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

#
#	Override defaults to build a unique, local modstubs.o.
#
CLOBBERFILES	+= $(MODULE)
CFLAGS		+= -v -DSUNDDI

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
