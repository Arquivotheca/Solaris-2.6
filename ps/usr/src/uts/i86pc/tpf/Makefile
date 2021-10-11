#
# uts/i86pc/tpf/Makefile
# Copyright (c) 1995 by Sun Microsystems, Inc. All rights reserved.
#
#ident	"@(#)Makefile	1.3	95/12/06 SMI"
#
#       This makefile drives the production of the tpf "misc"
#	kernel module.
#
#       tpf (TRICORD) implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#       Define the module and installation location.
#
MODULE		= tpf
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

