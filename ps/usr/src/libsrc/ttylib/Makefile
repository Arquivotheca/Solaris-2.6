#
#ident @(#)Makefile	96/05/07	1.8 SMI
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#	Makefile for maintaining usr/src/libsrc/ttylib
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

PRIVHDRS		= tty_color.h ttylib_msgs.h

PUBHDRS = tty_utils.h tty_form.h tty_help.h tty_formtypes.h

INSTPUBHDRS =	$(PUBHDRS:%.h=$(INCDESTDIR)/%.h)

SRCS		= ttylib_msgs.c tty_color.c tty_form.c tty_intro.c tty_help.c \
			tty_menu.c tty_utils.c tty_format.c 

.PARALLEL: $(OBJS)

OBJS		= $(SRCS:.c=.o)

LIBRARY		= libtty.a

INSTLIBRARY	= $(LIBRARY:%.a=$(LIBDESTDIR)/%.a)

.KEEP_STATE:

all:		msgs .WAIT $(LIBRARY) .WAIT install

$(LIBRARY):     $(OBJS)
		$(AR) $(ARFLAGS) $@ $(OBJS)

install:        tmp_dirs $(INSTLIBRARY) $(INSTPUBHDRS)

strip:

sccs:
		$(SCCSGET) $(SCCSGETFLAGS) $(SRCS) $(PRIVHDRS) $(PUBHDRS)

clean:		msgclean
		$(RM) $(OBJS) core *~ Makefile.bak

clobber:        clean
		$(RM) $(LIBRARY) $(INSTLIBRARY) $(INSTPUBHDRS) \
		    ttylib_msgs.[ch]

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

TEXT_DOMAIN=SUNW_INSTALL_TTYLIB

msgs: $(TEXT_DOMAIN).po
	@cp $(TEXT_DOMAIN).po ../messages

$(TEXT_DOMAIN).po ttylib_msgs.c:	msg_master
	@echo "${LEVEL}" 'Generating .po file and message sources from msg_master'
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
