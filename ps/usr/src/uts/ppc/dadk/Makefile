#
# uts/ppc/io/dadk/Makefile
# Copyright (c) 1991-1994, by Sun Microsystems, Inc.
#
#ident	"@(#)Makefile	1.5	96/07/17 SMI"
#
#	This makefile drives the production of the dadk "misc"
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
MODULE		= dadk
OBJECTS		= $(DADK_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(DADK_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_MISC_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/ppc/Makefile.ppc

#
#	Overrides.
#
CLOBBERFILES	+= $(MODULE)
DEBUG_FLGS	=
DEBUG_DEFS	+= $(DEBUG_FLGS)

# 
# lint pass one enforcement 
# 
CFLAGS += -v -Xt

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
include $(UTSBASE)/ppc/Makefile.targ
