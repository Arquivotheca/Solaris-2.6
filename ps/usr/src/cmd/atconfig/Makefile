#
#ident @(#)Makefile	1.2	96/05/07
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
# 	Makefile for maintaining usr/src/cmd/atconfig
#

include $(SRC)/Makefile.master

# Override POST_PROCESS_O as used in .c.o rule in Makefile.master so
# that we don't stamp each object, yet still retain the method of setting
# OBJS seen below.
POST_PROCESS_O  =

# passed in from upper level makefile for -g or -O optimization 
OPTFLAG= -O
MCSSTRING= $(RELEASE_CM)

SRCS= atconfig.c

CPPFLAGS=-I$(INCDESTDIR)
CFLAGS=${OPTFLAG} -Xa
LDFLAGS=

CMD=atconfig

.PARALLEL: $(OBJS)

OBJS=${SRCS:.c=.o}

OWNER=root
GROUP=sys
FILEMODES=0755

INSDIR=$(ROOT)/usr/sbin/install.d
INSTALLABLES=$(CMD:%=$(INSDIR)/%)

# install rules
$(INSDIR)/%: %
	$(INS.file)

.KEEP_STATE:

all:    ${CMD} 

# Degree of comment processing determined by assignments made to
# INTERNAL_RELEASE_BUILD and/or RELEASE_BUILD environment variables.
# See Makefile.master.

${CMD}: ${OBJS}
	${LINK.c} -o $@ ${OBJS} -lgen
	$(PROCESS_COMMENT) $(CMD)

install:    all .WAIT $(INSDIR) $(INSTALLABLES)

$(INSDIR):
	-$(INS.dir)

msgs msgs_test:

clean:
	-rm -f core errs *.o *.BAK ${CMD}

clobber: clean
	-rm -f ${CMD}
