#
# uts/prep/xsvc/Makefile
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved
#
#ident	"@(#)Makefile	1.6	96/07/12 SMI"
#
#       This makefile drives the production of the xsvc driver,
#	which enables user mode access to I/O space.
#
#       prep implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE		= ../..
SECRETBASE	= $(UTSBASE)/SunSoft_Confidential

#
#	Define the module and object file sets.
#
MODULE          = xsvc
OBJECTS         = $(XSVC_OBJS:%=$(OBJS_DIR)/%)
LINTS           = $(XSVC_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_PSM_DRV_DIR)/$(MODULE)
CONF_SRCDIR     = $(SECRETBASE)/prep/io

#
#	Include common rules.
#
include $(UTSBASE)/prep/Makefile.prep
 
#
#       Override defaults to build a unique, local modstubs.o.
#

CLOBBERFILES	+= $(MODULE)

#
#	Default build targets.
#
.KEEP_STATE:

all:		$(MODULE) $(SRC_CONFILE)

lint:		$(MODULE).lint


install:  	$(MODULE) $(ROOTMODULE) $(ROOT_CONFFILE)

#
#	Include common targets.
#
include $(UTSBASE)/prep/Makefile.targ
