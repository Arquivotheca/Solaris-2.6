#
#ident	"@(#)Makefile.com	1.2	94/11/03 SMI"
#
# Copyright (c) 1993 by Sun Microsystems, Inc.
#

include $(SRC)/Makefile.master

PKGARCHIVE=$(ROOT)/package
PKGDEFS=$(SRC)/pkg
DATAFILES=copyright
FILES=$(DATAFILES) pkginfo

PACKAGE:sh= basename `pwd`

CLOBBERFILES= $(FILES)
