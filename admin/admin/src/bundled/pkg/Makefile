#
#ident	"@(#)Makefile	1.14	96/07/30 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# pkg/Makefile
#
# include global definitions
include ./Makefile.com

sparc_SUBDIRS =

i386_SUBDIRS =

ppc_SUBDIRS =

COMMON_SUBDIRS	= SUNWinst SUNWcdbd SUNWcdrom SUNWadmr \
				SUNWadmap SUNWlpmsg SUNW0adm SUNWsibi SUNWswmt

SUBDIRS		= $(COMMON_SUBDIRS) $($(ARCH)_SUBDIRS)

all:=		TARGET= all
install:=	TARGET= install
patch:=		TARGET= patch
clean:=		TARGET= clean
clobber:=	TARGET= clobber
lint:=		TARGET= lint

%: %.ksh
	$(RM) $@
	cp $< $@
	chmod +x $@

.KEEP_STATE:

all install: awk_pkginfo $(SUBDIRS)

awk_pkginfo: bld_awk_pkginfo
	./bld_awk_pkginfo -m $(ARCH) -p "$(RELEASE)/$(VERSION)" -o $@

lint install_h:

clobber:	clean
	rm -rf $(PKGARCHIVE)

clean:	$(SUBDIRS)
	-rm -f bld_awk_pkginfo awk_pkginfo

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)
