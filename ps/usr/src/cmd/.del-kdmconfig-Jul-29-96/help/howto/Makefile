#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident @(#)Makefile	1.2	93/12/14
#
# ident  Derived from template: @(#)Makefile.program	1.5	92/09/24
#

include $(SRC)/Makefile.master

FILES           = Howto navigate.help

INSDIR=$(ROOT)/usr/lib/locale/C/help/kdmconfig.help/howto
INSTALLABLES=$(FILES:%=$(INSDIR)/%)

OWNER=root
GROUP=bin
FILEMODE=0444

# install rules
$(INSDIR)/%: %
	$(INS.file)

.KEEP_STATE:

all clean clobber cstyle depend lint lintlib strip:
	@true

install:        $(INSDIR) $(INSTALLABLES)
	@true

$(INSDIR):
	-$(INS.dir)

sccs:
		$(SCCSGET) $(SCCSGETFLAGS) $(FILES)

