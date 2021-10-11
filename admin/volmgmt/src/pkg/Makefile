#
# @(#)Makefile 1.2 95/02/01
#
#	Makefile to build SUNWvolg & SUNW0volg pkg's

PKGS=	SUNWvolg SUNW0volg

all:	$(PKGS)

install: $(PKGS)

SUNWvolg: FORCE
	cp common_files/copyright SUNWvolg
	cd $@; pwd; $(MAKE)

SUNW0volg: FORCE
	cp common_files/copyright SUNW0volg
	cd $@; pwd; $(MAKE)

clean:
	cd SUNWvolg; pwd; $(MAKE) clean
	cd SUNW0volg; pwd; $(MAKE) clean

clobber:
	cd SUNWvolg; pwd; $(MAKE) clobber
	cd SUNW0volg; pwd; $(MAKE) clobber

FORCE:
