#
# uts/prep/elx/Makefile
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#ident	"@(#)Makefile	1.12	96/07/17 SMI"
#
#	This makefile drives the production of the 'elx' driver 
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
MODULE		= elx
OBJECTS		= $(ELX_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(ELX_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/prep/io

#
#	Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep

#
#	Overrides.
#
CLOBBERFILES	+= $(MODULE)
INLINES		= $(UTSBASE)/ppc/ml/ppc.il

#
# lint pass one enforcement
#
CFLAGS += -v -Xt

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
include $(UTSBASE)/prep/Makefile.targ
