#
#ident  "@(#)Makefile 1.3 95/02/01 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
#
# The package name will be SUNWvolg

SUBDIRS=  gui
PACKAGE= pkg

all:=           TARGET= all
install:=       TARGET= install
lint:=          TARGET= lint
cstyle:=        TARGET= cstyle
clean:=         TARGET= clean
clobber:=       TARGET= clobber

.KEEP_STATE:

all install:      $(SUBDIRS) $(PACKAGE)

clean clobber: $(SUBDIRS) $(PACKAGE)

lint cstyle: $(SUBDIRS)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

$(PACKAGE): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
