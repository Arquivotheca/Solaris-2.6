#
#       @(#)Makefile 1.9 94/10/13 SMI
#
# Makefile for 50install/files
#

# FILES are those things that are just used as is

include ../Makefile.cmd

FILES= stub.4x.cpio

SCRIPTS=stub.4x.part2 stub.4x.check

PROG= $(FILES) $(SCRIPTS)

.KEEP_STATE:

all: ${PROG}
	@[ -f .stub.4x.part2 ] || cp stub.4x.part2 .stub.4x.part2
	@[ -f .stub.4x.check ] || cp stub.4x.check .stub.4x.check
	@[ -f .stub.4x.cpio ] || cp stub.4x.cpio .stub.4x.cpio

install: all $(ROOTPROG)

msgs msgs_test strip:

clean:
	-rm -f ${SCRIPTS}

clobber: clean
	-rm -f .stub*

include ../Makefile.targ
