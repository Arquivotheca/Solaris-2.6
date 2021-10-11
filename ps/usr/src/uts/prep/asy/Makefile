#
# uts/prep/asy/Makefile
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#pragma ident	"@(#)Makefile	1.10	96/07/17 SMI"
#
#	This makefile drives the production of the asy `drv'
#	kernel module.
#
#	prep implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= asy
OBJECTS		= $(ASY_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(ASY_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/io

#
#	Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep

#
#	Override defaults to build a unique, local modstubs.o.
#
CLEANFILES	+= $(MODSTUBS_O)
CLOBBERFILES	+= $(MODULE)
CPPFLAGS	+= -DPSARC_1993_408_CONSOLE

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
