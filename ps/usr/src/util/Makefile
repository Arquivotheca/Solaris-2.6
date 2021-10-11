#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident "@(#)Makefile 1.2 94/04/29"
#
# ident  Derived from tempate: @(#)Makefile.subdir	1.1	92/09/21
#

include ../Makefile.master

# passed in from upper level makefile for -g or -O optimization 
LEVEL="=="
OPTFLAG		= -g
CPOPTFLAG	= -g
MCSSTRING       = $(RELEASE_CM)\(\#\)Solaris\ 2.1\ FCS\ November\ 1992

CFLAGS		= ${OPTFLAG}

###
### Instructions:
###
### Fill in SUBDIRS for the subdirectories to descend into for the build.
###
SUBDIRS		= mkmsgs OWconfig
DSUBDIRS	= mkmsgs .WAIT OWconfig

all:=		TARGET = all
install:=       TARGET = install
clean:=		TARGET = clean
clobber:=       TARGET = clobber
cstyle:=	TARGET = cstyle
depend:=	TARGET = depend
lint:=		TARGET = lint
lintlib:=	TARGET = lintlib
msgs:=		TARGET = msgs
sccs:=		TARGET = sccs
sclean:=	TARGET = sclean

.KEEP_STATE:

all install clean clobber cstyle depend lintlib lint sccs: ${DSUBDIRS}
	@true

${SUBDIRS}: FRC
		@(cd $@; pwd; $(MAKE) 'LEVEL=${LEVEL}==' 'OPTFLAG=${OPTFLAG}' \
			'CPOPTFLAG=${CPOPTFLAG}' ${TARGET})

sclean: clean ${SUBDIRS}
	@true

msgs: rm_msgs .WAIT messages .WAIT ${SUBDIRS}
	@mv messages/* ../../messages
	@-rm -rf messages

rm_msgs: FRC
	@-rm -rf messages

messages: FRC
	@[ -d messages ] || (rm -f messages ;mkdir -p messages)

FRC:

.WAIT:
