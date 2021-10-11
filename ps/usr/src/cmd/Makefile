#
#ident @(#)Makefile	1.38	96/08/06
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
# The x86 directory contains x86 platform specific executables.  Only
# build this subtree if building for x86.
#

include $(SRC)/Makefile.master

# passed in from upper level makefile for -g or -O optimization 
LEVEL="=="
OPTFLAG		= -g
MCSSTRING       = $(RELEASE_CM)

CFLAGS		= ${OPTFLAG}

###
### Instructions:
###
### Fill in SUBDIRS for the subdirectories to descend into for the build.
###
i386_SUBDIRS		= rootprop bootargs devconfig pcfs_newfs \
			  scripts initpkg loadfont pcmapkeys
ppc_SUBDIRS		= diskprep atconfig vofcheck

SUBDIRS			= $($(MACH)_SUBDIRS)

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
strip:=		TARGET = strip

.KEEP_STATE:

all install clean clobber cstyle depend lint lintlib sccs strip: ${SUBDIRS}
	@true

${SUBDIRS}: FRC
	@(cd $@; pwd; $(MAKE) 'LEVEL=${LEVEL}==' \
		'MCSSTRING=${MCSSTRING}' 'OPTFLAG=${OPTFLAG}' \
		'CC=${CC}' 'CCC=${CCC}' ${TARGET}); \

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
