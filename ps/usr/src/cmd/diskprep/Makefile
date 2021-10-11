#
#ident "@(#)Makefile 1.1 95/05/08"
#
# Copyright (c) 1995 Sun Microsystems, Inc.  
# All Rights Reserved.
#
# Makefile for cmd/diskprep
#

include $(SRC)/Makefile.master

# SCRIPTS are .sh files, use the default rules
# FILES are those things that are just used as is
ISCRIPTS=prep_partition
SCRIPTS=$(ISCRIPTS)

INSTALLD=$(ROOT)/usr/sbin/install.d
INSDIRS=$(INSTALLD)

OWNER=root
#GROUP=sys

INSTALLABLES=$(ISCRIPTS:%=$(INSTALLD)/%)

$(INSTALLD)/$(ISCRIPTS):= FILEMODE=0500

# install rules
$(INSTALLD)/%: %
	$(INS.file)

.KEEP_STATE:

all: ${SCRIPTS}

install:    all $(INSDIRS) $(INSTALLABLES)

$(INSDIRS):
	-$(INS.dir)

strip:
	@true

clean:
	@

clobber: clean

