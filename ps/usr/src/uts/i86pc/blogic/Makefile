#
# uts/i86pc/blogic/Makefile
# Copyright (c) 1994-1995 by Sun Microsystems, Inc.
# All rights reserved.
#
#pragma ident	"@(#)Makefile	1.6	95/06/30 SMI"
#
#	This makefile drives the production of the blogic "drv"
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
MODULE		= blogic
OBJECTS		= $(BLOGIC_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(BLOGIC_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/i86pc/io/dktp/hba/blogic

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

DEBUG_FLGS      = -DBLOGIC_DEBUG
$(NOT_RELEASE_BUILD)DEBUG_DEFS	+= $(DEBUG_FLGS)
# EXTRA_OPTIONS and EXTRA_CFLAGS are the correct variables to which
# driver-specific cpp and compiler flags should be appended to the cc
# command (see ../../Makefile.uts and ../../../Makefile.master)
EXTRA_OPTIONS   +=
EXTRA_CFLAGS    +=
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
