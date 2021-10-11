#
# uts/i86pc/apic/Makefile
# Copyright (c) 1994 by Sun Microsystems, Inc.
#
#ident	"@(#)Makefile	1.2	94/03/02 SMI"
#
#       This makefile drives the production of the apic "misc"
#	kernel module.
#
#       apic implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#       Define the module and installation location.
#
MODULE		= pcplusmp
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

