# 
# Makefile for swmtool.
#
# This Makefile builds the _stubs.c, _ui.c, _ui.h, and _info
# files from Guide .G files and transfers them to the main
# build directory (..).
#
#      "@(#)Makefile 1.10 93/11/10"
#
# Parameters.

LEVEL="=="
TEXT_DOMAIN = SUNW_INSTALL_SWM

GXV	= $(GUIDEHOME)/bin/gxv

BUILDDIR = ..

PROGRAM = swmtool
SOURCES.c = swmtool.c
SOURCES.h = swmtool.h

# Read the names of the .G files from the .P file.
SOURCES.G:sh= grep :interfaces swmtool.P | sed 's/^.*:interfaces *(\(.*\))/\1/'
STUBS.G:sh= grep :interfaces swmtool.P | sed 's/^.*:interfaces *(\(.*\))/\1/'

# Derived parameters.

SOURCES = \
	$(SOURCES.G) \
	$(SOURCES.h) \
	$(SOURCES.l) \
	$(SOURCES.y) \
	$(SOURCES.cps) \
	$(SOURCES.c)

TARGETS.c = \
	$(SOURCES.G:%.G=%_ui.c) \
	$(STUBS.G:%.G=%_stubs.c)

TARGETS.h = \
	$(SOURCES.G:%.G=%_ui.h) \
	$(SOURCES.l:%.l=%.h) \
	$(SOURCES.y:%.y=%.h) \
	$(SOURCES.cps:%.cps=%.h)

TARGETS.other = \
	$(SOURCES.G:%.G=%.info)

TARGETS = \
	$(TARGETS.other) \
	$(TARGETS.h) \
	$(TARGETS.c) \
	$(PROGRAM).info

# Compiler flags

SCCSGET		= sccs get

# Standard targets.

.KEEP_STATE:

all:	 icons $(SOURCES) $(TARGETS)
	@true

sources: $(SOURCES)
	@true

targets: $(SOURCES) $(TARGETS)
	@true

icons:
	ln -s $(BUILDDIR)/icons icons

install:	all
	cp $(TARGETS) $(BUILDDIR)

clean:
	$(RM) $(SOURCES.G:%.G=%_ui.c) $(SOURCES.G:%.G=%_stubs.c) \
		$(SOURCES.c) $(SOURCES.h) $(TARGETS.other) $(TARGETS.h) \
		$(PROGRAM).info $(OBJECTS) *.BAK *.delta core icons

$(PROGRAM).c: $(PROGRAM).P $(SOURCES.G)
	@rm -f $(PROGRAM).c.BAK $(PROGRAM).c.delta $(PROGRAM).[ch]
	$(GXV) -gettext -domain $(TEXT_DOMAIN) -project $(PROGRAM) -main
	@if [ -f $(PROGRAM).sed ]; then \
		echo ${LEVEL} sed: editing $(PROGRAM).c; \
		sed -f all.sed -f $(PROGRAM).sed $(PROGRAM).c > $(PROGRAM).c.sed; \
		echo ${LEVEL} sed: writing $(PROGRAM).c; \
		mv $(PROGRAM).c.sed $(PROGRAM).c; \
	fi

$(PROGRAM).h: $(PROGRAM).c
	@touch $@

$(PROGRAM).info:  $(TARGETS.other)
	cat $(TARGETS.other) > $(PROGRAM).info

%_ui.c: %.G
	@rm -f `basename $* .G`*.*.BAK `basename $* .G`*.*.delta `basename $* .G`*.[ch]
	$(GXV) -gettext -domain $(TEXT_DOMAIN) -project $(PROGRAM) $*
	@for ext in _stubs _ui; do \
		name=`basename $* .G`$${ext}; \
		if [ -f $${name}.sed ]; then \
			echo ${LEVEL} sed: editing $${name}.c; \
			sed -f all.sed -f $${name}.sed $${name}.c > $${name}.c.sed; \
			echo ${LEVEL} sed: writing $${name}.c; \
			mv $${name}.c.sed $${name}.c; \
		else \
			echo ${LEVEL} sed: editing $${name}.c; \
			sed -f all.sed $${name}.c > $${name}.c.sed; \
			echo ${LEVEL} sed: writing $${name}.c; \
			mv $${name}.c.sed $${name}.c; \
		fi; \
		if [ -f $${name}.h ]; then \
			echo ${LEVEL} sed: editing $${name}.h; \
			sed -f all.sed $${name}.h > $${name}.h.sed; \
			echo ${LEVEL} sed: writing $${name}.h; \
			mv $${name}.h.sed $${name}.h; \
		fi; \
	done

%_ui.h: %_ui.c
	@touch $@

%_stubs.c: %_ui.c
	@touch $@

%.info: %_ui.c
	@touch $@
