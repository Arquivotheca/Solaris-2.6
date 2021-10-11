#
# uts/i86pc/ncrs/Makefile
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#ident @(#)Makefile	1.5	96/02/12 SMI
#
#	This makefile drives the production of the ncrs "drv"
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
MODULE		= ncrs
OBJECTS		= $(NCRS_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(NCRS_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/io/dktp/hba/ncrs

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

DEBUG_FLGS	=  
DEBUG_DEFS	+= $(DEBUG_FLGS)
CLOBBERFILES	+= $(MODULE)

#
#	Default build targets.
#
.KEEP_STATE:

.PARALLEL: $(OBJECTS)

all:		$(MODULE)

lint:		$(MODULE).lint

install:  	$(MODULE) $(ROOTMODULE) $(ROOT_CONFFILE)

#
#	Include common targets.
#
include $(UTSBASE)/i86pc/Makefile.targ
