#
#ident	"@(#)Makefile	1.3	93/08/16 SMI"
#
# Copyright (c) 1992 by Sun Microsystems, Inc.
#
# cmd/mkmsgs/Makefile
#
PROG=		adm_mkmsgs

SHFILES =

TXTS =

OBJECTS=	mkmsgs.o

SRCS=		$(OBJECTS:.o=.c)

include ../Makefile.cmd

# where to find the header files
CPPFLAGS=	$(CPPFLAGS.master)

# where to find the libraries
# what gets installed, and where

.KEEP_STATE:

all:		$(PROG)

$(PROG):	$(OBJECTS)
		$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)
		$(POST_PROCESS)

install:	all rmprog $(ROOTADMINBINPROG)

install_h:

rmprog:
	@$(RM) -f $(ROOTADMINBINPROG)

lint:		lint_SRCS

clean:
	$(RM) $(OBJECTS)

include ../Makefile.targ
