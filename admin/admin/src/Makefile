#
#ident	"@(#)Makefile 1.5 95/05/15 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved
#
# Makefile for SNAG application source
#
# include global definitions
include ./Makefile.master
#
# the Targetdirs file is the AT&T target.dirs file in a makefile format.
# it defines TARGETDIRS and ROOTDIRS.
include Targetdirs
SUBDIRS:sh =if [ -d "unbundled"  ]; then \
                echo "common .WAIT unbundled" ; \
          else  echo "common .WAIT bundled"; \
          fi

$(CH)PACKAGE=	pkg

all:=		TARGET=	all
all:=		PKTARG=	all
install:=	TARGET=	install
install:=	PKTARG=	install
patch:=		TARGET=	install
patch:=		PKTARG=	patch
newpatch:=	TARGET=	install
newpatch:=	PKTARG=	patch
clean:=		TARGET=	clean
clean:=		PKTARG=	clean
clobber:=	TARGET=	clobber
clobber:=	PKTARG=	clobber

.KEEP_STATE:

all install:	headers .WAIT $(SUBDIRS)

clean clobber: $(SUBDIRS)

proto: headers
	cd common; pwd; $(MAKE) install

headers: $(ROOTDIRS) .WAIT proto-setup .WAIT
	@for i in ${SUBDIRS}; do \
		if [ -d $${i} ]; then \
			(cd $${i}; pwd; $(MAKE) headers;)\
		fi; \
	done;

$(SUBDIRS): FRC
	@if [ -d $@ ]; then \
		cd $@; pwd; $(MAKE) $(TARGET);\
	fi

$(PACKAGE): FRC
	@cd $@; pwd; $(MAKE) $(PKTARG)

$(ROOTDIRS):
	$(INS.dir)

# Used when ROOT is not /, to set up links to the
# system header files.
# PROTO should point to where the SVr4 header files
# are.  For native builds, this is /usr/include,
# so normally you'd just not set PROTO.
proto-setup: $(ROOTDIRS)
	@if [ "${ROOT}" != "/" ] ; \
	then \
		cd ${ROOT}/usr/include ;\
		for i in ${PROTO}/usr/include/* ;\
		do \
			if [ `basename $${i}` != "admin" ] ;\
			then \
				test -h `basename $${i}` || $(SYMLINK) $$i `basename $${i}` ;\
			fi ;\
		done ;\
		cd ${SRC} ;\
	fi

FRC:
