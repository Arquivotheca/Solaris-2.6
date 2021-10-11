#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile 1.15 95/03/14 SMI"
# mntpnt makefile
#

include ../../Makefile.cmd

SRCS=	get_mntpnt.c \
	set_mntpnt.c 

CPPFLAGS=
CFLAGS		+= $(DEBUG_CFLAGS) -Xa

PROG=${SRCS:%.c=%}

${ARCH}/%: %.c
	${LINK.c} -o $@ $<

.KEEP_STATE: 

all: ${ARCH} .WAIT $(PROG)

install: all $(ROOTINSTALLDPROG)

${ARCH}:
	@[ -d ${ARCH} ] || (rm -f ${ARCH} ;mkdir -p ${ARCH})

${PROG}: ${SRCS}
	$(LINK.c) -o $@ $@.c
	cp $@ ${ARCH}
	$(POST_PROCESS)

msgs:

lint:
	lint -m -x ${INCLUDE} ${SRCS}

clean:
	-rm -f core errs

clobber: clean
	-rm -f ${PROG}
	-rm -rf ${ARCHSUPPORT}

include ../../Makefile.targ

.WAIT:
