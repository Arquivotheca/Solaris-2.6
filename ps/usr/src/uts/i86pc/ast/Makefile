#
# uts/i86pc/ast/Makefile
# Copyright (c) 1995 by Sun Microsystems, Inc.
#
#ident	"@(#)Makefile	1.1	95/04/14 SMI"
#
#       This makefile drives the production of the ast "mach"
#	kernel module.
#
#       ast implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#       Define the module and installation location.
#
MODULE          = ast
ROOTMODULE	= $(ROOT_MACH_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc
#
#	Default build targets.
#
.KEEP_STATE:

all:

lint:

install:        $(MODULE) $(ROOTMODULE)

clean:

clobber:

#
#	Include common targets.
#
#include from $(UTSBASE)/i86pc/Makefile.targ:
$(ROOT_MOD_DIR): $(BASE_INS_DIR)

$(BASE_INS_DIR) $(ROOT_MOD_DIRS):
	-$(INS.dir)

$(ROOT_MACH_DIR)/%:	% $(ROOT_MOD_DIR) $(ROOT_MACH_DIR)
	$(INS.file)

