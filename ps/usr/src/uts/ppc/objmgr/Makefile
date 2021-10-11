#
# uts/ppc/io/objmgr/Makefile
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#pragma ident "@(#)Makefile 1.1	94/04/13 SMI"
#
#	This makefile drives the production of the objmgr "drv"
#	kernel module.
#
#	ppc architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= objmgr
OBJECTS		= $(OBJMGR_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(OBJMGR_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/ppc/io/dktp/drvobj

#
#	Include common rules.
#
include $(UTSBASE)/ppc/Makefile.ppc

DEBUG_FLGS	=
DEBUG_DEFS	+= $(DEBUG_FLGS)
CLOBBERFILES	+= $(MODULE)

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
include $(UTSBASE)/ppc/Makefile.targ
