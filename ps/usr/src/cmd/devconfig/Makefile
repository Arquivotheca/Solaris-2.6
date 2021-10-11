#
#ident @(#)Makefile	1.40	96/05/07 SMI
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#	Makefile for maintaining usr/src/cmd/devconfig
#

include $(SRC)/Makefile.master

# Override POST_PROCESS_O as used in .c.o rule in Makefile.master so
# that we don't stamp each object, yet still retain the method of setting
# OBJS seen below.
POST_PROCESS_O  =

# passed in from upper level makefile for -g or -O optimization 
OPTFLAG		= 
MCSSTRING       = $(RELEASE_CM)

LD=$(CCC)

INCLUDES        += -I$(INCDESTDIR)

LIBDIR1         = $(LIBDESTDIR)
LIBDIRS		+= -L${LIBDIR1}

SCCSGET         = sccs get
CPPFLAGS        = ${INCLUDES}
$(NOT_RELEASE_BUILD)CPPFLAGS    += -DDEBUG
CFLAGS		= ${OPTFLAG}
CCFLAGS         += ${INCLUDES}
$(NOT_RELEASE_BUILD)CCFLAGS     += -DDEBUG
LINTFLAGS	+= ${INCLUDES}
LDFLAGS		+= ${LIBDIRS}

SUBDIRS         = support

all:=           TARGET = all
install:=       TARGET = install
clean:=         TARGET = clean
sccs:=          TARGET = sccs

###
### Instructions:
###
### Fill in HDRS, SRCS, LIBS, and PROGRAM name to build the program.
### LIBS should use the -l<libname> form rather than explicit references
### to .a files, and allow the linker to find library files via the
### -L flags specified above.
###
PUBHDRS		=
PRIVHDRS	= 

HDRS		= ${PRIVHDRS} ${PUBHDRS}

SRCS		= devcfg_msgs.c cui.c

OBJS		= $(SRCS:.c=.o)

LIBS    = -ldvc -lcui -letibugs -lform -lmenu -lpanel -lcurses -lkvm -lgen -lelf -ldl -lc -Bstatic

PROGRAM =devconfig

DEPLIBS = ${LIBDIR1}/libcui.a ${LIBDIR1}/libetibugs.a ${LIBDIR1}/libdvc.a

FILEMODE=0755

INSDIR=$(ROOT)/usr/sbin
INSTALLABLES=$(PROGRAM:%=$(INSDIR)/%)

# install rules
$(INSDIR)/%: %
	$(INS.file)

.KEEP_STATE:

${SUBDIRS}: FRC
	@if [ "`uname -m`" = "i86pc" ]; then \
		(cd $@; pwd; $(MAKE) "SCCSGET=$(SCCSGET)" ${TARGET}); \
	fi

all:    ${PROGRAM}

.PARALLEL: $(OBJS)

# Degree of comment processing determined by assignments made to
# INTERNAL_RELEASE_BUILD and/or RELEASE_BUILD environment variables.
# See Makefile.master.

$(PROGRAM):	$(OBJS) $(DEPLIBS)
		$(LINK.cc) -o $@ $(OBJS) $(LIBS)
		$(PROCESS_COMMENT) $(PROGRAM)

install:        all .WAIT $(INSDIR) $(INSTALLABLES) $(SUBDIRS)

$(INSDIR):
	-$(INS.dir)

sccs:           $(SUBDIRS)
		-$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(HDRS)

clean:          msgclean $(SUBDIRS)
		$(RM) $(OBJS) $(PROGRAM) *.bak tags multi.gather devcfg_msgs.[ch]

clobber:        clean
		$(RM) $(PROGRAM)

msgclean:
		@rm -f *.[pm]o

sclean:		clean
		sccs clean

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		cstsyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

TEXT_DOMAIN=SUNW_INSTALL_DEVCFG

msgs: $(TEXT_DOMAIN).po
	@cp $(TEXT_DOMAIN).po ../messages

$(TEXT_DOMAIN).po devcfg_msgs.c:	msg_master
	@echo ${LEVEL} 'Generating .po file and devcfg_msgs.[ch] from msg_master'
	@$(MKMSGS) -d $(TEXT_DOMAIN) < msg_master

make-mo:
	sccs get make-mo
	chmod +x ./make-mo

mofile: $(TEXT_DOMAIN).po make-mo
	@echo '${LEVEL} Making dummy-de .mo file'
	@./make-mo de
	@msgfmt -o $(TEXT_DOMAIN).mo de.po
	@rm -f de.[pm]o

# 'makedepend' requires /usr/openwin/bin in path.
depend:		$(SRCS) $(HDRS)
		makedepend $(INCLUDES) $(SRCS)

FRC:

# DO NOT DELETE THIS LINE -- make depend depends on it.
