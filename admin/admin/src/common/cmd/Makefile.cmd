#
#ident	"@(#)Makefile.cmd	1.2	94/10/20 SMI"
#
# Copyright (c) 1992 by Sun Microsystems, Inc.
#
#	cmd/Makefile.cmd prototype with NSE compatibility in mind.
#	Definitions common to command source.
#
# include global definitions; SRC should be defined in the shell.
include $(SRC)/Makefile.master

FILEMODE=	0755
LIBFILEMODE=	0644
ROOTADMINBIN = 	$(ROOTADMIN)/bin

# storing LDLIBS in two macros allows reordering of options
# LDFLAGS += -s

ROOTADMINBINPROG = $(PROG:%=$(ROOTADMINBIN)/%)
