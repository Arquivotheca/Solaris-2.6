#
#ident "@(#)Makefile 1.12 95/03/09"
#
# Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
# considers its source code as an unpublished, proprietary trade secret, and
# it is available only under strict license provisions.  This copyright
# notice is placed here only to protect Sun in the event the source is
# deemed a published work.  Dissassembly, decompilation, or other means of
# reducing the object code to human readable form is prohibited by the
# license agreement under which this code is provided to the user or company
# in possession of this copy.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
# is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
# Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
# and in similar clauses in the FAR and NASA FAR Supplement.
#
# Makefile for cmd/scripts
#

include $(SRC)/Makefile.master

# SCRIPTS are .sh files, use the default rules
# FILES are those things that are just used as is
ETCSCRIPTS=setdefaultkb
ISCRIPTS=atconfig
SCRIPTS=$(ETCSCRIPTS) $(ISCRIPTS)
FILES=bootrc.cd bootrc.install

ETC=$(ROOT)/etc
INSTALLD=$(ROOT)/usr/sbin/install.d
INSDIRS=$(ETC) $(INSTALLD)

OWNER=root
GROUP=sys

INSTALLABLES=$(ETCSCRIPTS:%=$(ETC)/%)      \
	     $(ISCRIPTS:%=$(INSTALLD)/%)  \
	     $(FILES:%=$(ETC)/%)

$(ETC)/$(FILES):=   FILEMODE=0644
$(ETC)/$(ETCSCRIPTS) $(INSTALLD)/$(ISCRIPTS):= FILEMODE=0755

# install rules
$(ETC)/% $(INSTALLD)/%: %
	$(INS.file)

.KEEP_STATE:

all: ${FILES} ${SCRIPTS}

install:    all $(INSDIRS) $(INSTALLABLES)

$(INSDIRS):
	-$(INS.dir)

#
# NOTE: atconfig isn't a script anymore...
#
atconfig: atconfig.o
	$(CC) -o atconfig -O atconfig.o -lgen

strip:
	@true

clean:
	-rm -f ${SCRIPTS} *.po *.mo

clobber: clean

