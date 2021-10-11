#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.17 95/05/04 SMI"
# bootargs makefile

include ../../Makefile.cmd

SRCS=	getbootargs.c

OBJS =	${SRCS:%.c=${ARCH}/%.o}

CFLAGS	+= $(DEBUG_CFLAGS) -Xa
LDFLAGS	+= $(DEBUG_CFLAGS)

PROG=	getbootargs

${ARCH}/%.o: %.c
	${COMPILE.c} -o $@ $<

.KEEP_STATE:

all:	${ARCH} .WAIT ${PROG} 
	@true

${ARCH}:
	@[ -d ${@} ] || (${RM} ${@} ;mkdir -p ${@})

${PROG}: ${OBJS}
	${LINK.c} -o $@ ${OBJS}
	cp $@ ${ARCH}
	$(POST_PROCESS)

install: all $(ROOTSBINPROG)

msgs msgs_test:
	@true

clean:
	-${RM} core errs *.BAK ${OBJS} *.o

clobber: clean
	-${RM} ${PROG}
	-${RM} -r ${ARCHSUPPORT}

include ../../Makefile.targ
