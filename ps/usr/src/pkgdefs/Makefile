#
#ident	"@(#)Makefile	1.26	96/07/17 SMI"
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
#

include $(SRC)/Makefile.master

COMMON_SUBDIRS= \
	SUNWpcelx \
	SUNWpcmem \
	SUNWpcser

i386_SUBDIRS= \
	SUNWcdx86 \
	SUNWkey \
	SUNWos86r \
	SUNWos86u \
	SUNWpsdcr \
	SUNWpsder \
	SUNWpsdir \
	SUNWpsdmr \
	SUNWpsh \
	SUNWshlib

sparc_SUBDIRS=

ppc_SUBDIRS= \
	SUNWcdx86 \
	SUNWos86r \
	SUNWos86u \
	SUNWpsdcr \
	SUNWpsdir \
	SUNWpsh

SUBDIRS= $($(MACH)_SUBDIRS) $(COMMON_SUBDIRS)


all :=         		TARGET= all
install := 		TARGET= install
install_h :=            TARGET= install_h
clean :=                TARGET= clean
clobber :=              TARGET= clobber

%: %.ksh
	$(RM) $@
	cp $< $@
	chmod +x $@

.KEEP_STATE:

.PARALLEL: $(SUBDIRS)

#ttt:	FRC
#	@for i in SUNWkvm.i SUNWcar.i; do cd $$i; make -i install; cd ..; done

all install: awk_pkginfo $(SUBDIRS)
clean clobber: $(SUBDIRS)
	$(RM) awk_pkginfo bld_awk_pkginfo

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) -i $(TARGET)

awk_pkginfo: bld_awk_pkginfo
	./bld_awk_pkginfo -m $(MACH) -p "$(RELEASE)/$(VERSION)" -o $@

FRC:
