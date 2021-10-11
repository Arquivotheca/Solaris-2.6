#
# uts/i86pc/mp/Makefile
# Copyright (c) 1993 by Sun Microsystems, Inc.
#
#pragma ident	"@(#)Makefile	1.1	93/11/22 SMI"
#
#	This makefile drives the production of the mp support files
#
#	i86pc  architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Include common rules.
#
include $(UTSBASE)/i86pc/Makefile.i86pc

#
#	Define the module and object file sets.
#
SRCDIR		= $(UTSBASE)/i86pc/os
MACHFILES	= mach
FILEMODE	= 644
OWNER		= root
GROUP		= sys
ROOTETC		= $(ROOT)/etc

SRCFILES	= $(MACHFILES:%=$(SRCDIR)/%)
ROOTMACHFILES	= $(MACHFILES:%=$(ROOTETC)/%)

#
#	Default build targets.
#
.KEEP_STATE:

all: $(SRCFILES)

install: all $(ROOTMACHFILES)

$(ROOTETC)/%: $(ROOTETC) $(SRCDIR)/%
	$(INS.file)

$(ROOT_MACH_DIR)/%: $(ROOT_MACH_DIR) $(SRCDIR)/%
	$(INS.file)

$(ROOTETC):
	$(INS.dir)

# 
#	No lints, so keep the clobber and clean targets reasonable. 
# 
CLEANLINTFILES	= Nothing_to_remove
CLEANFILES	= Nothing_to_remove
CLOBBERFILES	= Nothing_to_remove

#
#	Include common targets.
#
include $(UTSBASE)/i86pc/Makefile.targ
