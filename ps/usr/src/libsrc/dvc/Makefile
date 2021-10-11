#
#ident @(#)Makefile	1.24	96/05/07 SMI
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#	Makefile for maintaining usr/src/libsrc/dvc
#

include ../Makefile.lib

# Override POST_PROCESS_O as used in .c.o rule in Makefile.master so
# that we don't stamp each object, yet still retain the method of setting
# OBJS seen below.
POST_PROCESS_O  =

# passed in from upper level makefile for -g or -O optimization 
LEVEL="=="
OPTFLAG		= -g
CPOPTFLAG	= -g
MCSSTRING       = $(RELEASE_CM)

INCDIR1         = $(INCDESTDIR)
INCLUDES	= -I$(INCDIR1)

SCCSGET		= sccs get
CFLAGS		= ${CPOPTFLAG} ${INCLUDES}
LINTFLAGS	= ${INCLUDES}
LDFLAGS		= ${CCFLAGS} ${LIBDIRS}

MODE		= 664
INSTALL		= $(INS)
INSTALLFLAGS	= -s -m $(MODE)

###
### Instructions:
###
### Fill in PRIVHDRS, PUBHDRS, SRCS, and LIBRARY name to build the library.
### PRIVHDRS = library private include (.h) files.  These files will
### be left in the current directory, and used only by this library.
### PUBHDRS  = library interface definition (protos, typedefs) files.
### These files will be installed in INCDESTDIR, to be accessed by
### programs that link this library.

PRIVHDRS =	conf.h dev.h util.h win.h

PUBHDRS =	dvc.h

INSTPUBHDRS =	$(PUBHDRS:%.h=$(INCDESTDIR)/%.h)

SRCS =		dvc_msgs.c chk.c cat.c \
		conf.c conf_attr.c conf_file.c conf_parse.c \
		dev.c dev_ops.c devi.c \
		mod.c pmi.c typ.c util.c win.c win_ops.c xmalloc.c 

.PARALLEL: $(OBJS)

OBJS		= $(SRCS:.c=.o)

LIBRARY		= libdvc.a

INSTLIBRARY	= $(LIBRARY:%.a=$(LIBDESTDIR)/%.a)

.KEEP_STATE:

all:		msgs .WAIT $(LIBRARY) .WAIT install

$(LIBRARY):     $(OBJS)
		$(AR) $(ARFLAGS) $@ $(OBJS)

install:        tmp_dirs $(INSTLIBRARY) $(INSTPUBHDRS)
		@(cd cfinfo; make install)

strip:

sccs:
		$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(PRIVHDRS) $(PUBHDRS)

clean:		msgclean
		$(RM) $(OBJS) core *~ Makefile.bak

clobber:        clean
		$(RM) $(LIBRARY) $(INSTLIBRARY) $(INSTPUBHDRS)
		@(cd cfinfo; make clobber)

sclean:		clean
		sccs clean

lint:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		cstsyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

# 'makedepend' requires /usr/openwin/bin in path.
depend:		$(SRCS) $(PRIVHDRS) $(PUBHDRS)
		makedepend $(INCLUDES) $(SRCS)

#
# Rules that make the installed library and includes depend on the
# files in the current directory, so that they don't get installed
# unless they are new.
#

$(LIBDESTDIR)/%.a:	%.a
	$(INSTALL) $(INSTALLFLAGS) -f $(LIBDESTDIR) $<

$(INCDESTDIR)/%.h:	%.h
	$(INSTALL) $(INSTALLFLAGS) -f $(INCDESTDIR) $<

TEXT_DOMAIN=SUNW_INSTALL_DVC

msgs: $(TEXT_DOMAIN).po
	@cp $(TEXT_DOMAIN).po ../messages

$(TEXT_DOMAIN).po dvc_msgs.c:	msg_master
	@echo "${LEVEL}" 'Generating .po file and dvc_msgs.[ch] from msg_master'
	@$(MKMSGS) -d $(TEXT_DOMAIN) < msg_master

make-mo:
	sccs get make-mo
	chmod +x ./make-mo

mofile: $(TEXT_DOMAIN).po make-mo
	@echo "${LEVEL}" 'Making dummy-de .mo file'
	@./make-mo de
	@msgfmt -o $(TEXT_DOMAIN).mo de.po
	@rm -f de.[pm]o

msgclean:
	rm -f dvc_msgs.[ch] $(TEXT_DOMAIN).po

.WAIT:

# DO NOT DELETE THIS LINE -- make depend depends on it.
