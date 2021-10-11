#
#      "@(#)Makefile 1.16 96/01/23"
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
# Makefile for jumpstart_sample files
#

include ../Makefile.cmd

FILES= rules any_machine host_class net924_sun4c rules upgrade x86-begin \
	x86-class set_root_pw

SCRIPTS=check

PROG=$(FILES) $(SCRIPTS)

SUBDIRS=x86-begin.conf

install:=	TARGET = install

.KEEP_STATE:

all: ${PROG}

install: all $(ROOTAIPROG) $(SUBDIRS)

${SUBDIRS}: FRC
	cd $@; pwd; $(MAKE) $(TARGET)

msgs msgs_test strip:

clean:

clobber:
	-rm -f ${SCRIPTS} *.ok

include ../Makefile.targ
