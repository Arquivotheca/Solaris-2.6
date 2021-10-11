#
# uts/prep/vuidps2/Makefile
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#ident	"@(#)Makefile	1.7	94/11/04 SMI"
#
#	This makefile drives the production of the vuidps2 streams kernel
#	module.
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
MODULE		= vuidps2
OBJECTS		= $(VUIDPS2_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(VUIDPS2_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_STRMOD_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep

#
#	Overrides.
#
CLOBBERFILES	+= $(MODULE)
ALL_DEFS	+= -DVUIDPS2

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
