#
# @(#)Makefile 1.14 94/10/21
#

PROG = xgetsh

SRCS= xgetsh.c nhash.c ncmsg.c

HDRS= nhash.h xgetsh.h

CFLAGS += $(DEBUG_CFLAGS) -Xa

LINTERR=lint_errors

OBJS=${SRCS:%.c=%.o}

include ../Makefile.cmd

LIBS=-lintl

.KEEP_STATE:

all: ${PROG} 
	@true

install: all rmprog $(ROOTADMINBINPROG)

${PROG}: ${OBJS} ${HDRS}
	$(LINK.c) -o $@ $(OBJS) $(LIBS)

rmprog:
	@$(RM) -f $(ROOTADMINBINPROG)

${OBJS}:	${HDRS}

lint:  ${SRCS} ${HDRS}
	lint ${CFLAGS} ${SRCS} >${LINTERR}

cleanlint:
	-rm ${LINTERR}

clean:
	-rm -f core errs *.o *.BAK ${LINTERR} ${OBJS}

include ../Makefile.targ

.WAIT:
