#
#ident	"@(#)Makefile	1.10	96/10/11 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# pkg/Makefile
#
# include global definitions
include ./Makefile.com

SUBDIRS= SUNWadmfw SUNWsadml SUNWadmc SUNWadmgn

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
	$(RM) bld_awk_pkginfo awk_pkginfo
	rm -rf $(PKGARCHIVE)

clean:	$(SUBDIRS)
	-rm -f bld_awk_pkginfo awk_pkginfo

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)
