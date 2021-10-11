#
# uts/ppc/sccd_audio/Makefile
# Copyright (c) 1996, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident	"@(#)Makefile	1.3	96/05/27 SMI"
#
#	This makefile drives the production of the sccd_audio "misc"
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
MODULE		= sccd_audio
OBJECTS		= $(SCCD_AUDIO_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(SCCD_AUDIO_OBJS:%.o=$(LINTS_DIR)/%.ln)
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
CFLAGS += -v 

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
