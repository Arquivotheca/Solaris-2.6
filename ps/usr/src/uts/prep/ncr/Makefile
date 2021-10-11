#
#
# ident	"@(#)Makefile	1.2	94/05/10 SMI"
#
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#	This makefile drives the production of the ncr "drv"
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
MODULE		= ncr
OBJECTS		= $(NCR_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(NCR_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/io/dktp/hba/ncr

#
#	Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep

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
include $(UTSBASE)/prep/Makefile.targ
