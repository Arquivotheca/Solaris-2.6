#
#ident  "@(#)Makefile 1.3 95/03/21 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved
#

PROG = genmsg

SRCS= 

HDRS=

CFLAGS=

LINTERR=lint_errors

OBJS=${SRCS:%.c=%.o}

include ../Makefile.cmd

LIBS=-lintl

.KEEP_STATE:

all: ${PROG} 
	@true

install: all rmprog $(ROOTADMINBINPROG)

${PROG}: docp

rmprog:
	@$(RM) -f $(ROOTADMINBINPROG)

${OBJS}:	${HDRS}

lint:

cleanlint:
	-rm ${LINTERR}

clean:
	-rm -f core errs *.o *.BAK ${LINTERR} ${OBJS}

docp:
	@if [ "`uname -p`" = "sparc" ]; then \
		${RM} ${PROG}; ${CP} ${PROG}.sparc ${PROG};  \
         elif [ "`uname -p`" = "i386" ]; then \
                 ${RM} ${PROG}; ${CP} ${PROG}.i386 ${PROG}; \
         elif [ "`uname -p`" = "ppc" ]; then \
                 ${RM} ${PROG}; ${CP} ${PROG}.ppc ${PROG}; \
	 elif [ 1 ]; then \
		echo "error building genmsg"; exit 1; \
         fi

include ../Makefile.targ

.WAIT:
