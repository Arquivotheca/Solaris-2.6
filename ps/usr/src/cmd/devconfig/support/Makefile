#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident @(#)Makefile	1.1	93/12/06
#
# ident  Derived from template: @(#)Makefile.program	1.5	92/09/24
#

include $(SRC)/Makefile.master

FILES1          = devconfig.info
FILES2          = cui.cui
FILES           = $(FILES1) $(FILES2)

INSDIR1=$(ROOT)/usr/lib/devconfig/lib/locale/C/help
INSDIR2=$(ROOT)/usr/lib/devconfig/lib
INSTALLABLES=$(FILES1:%=$(INSDIR1)/%) $(FILES2:%=$(INSDIR2)/%)

OWNER=root
GROUP=sys
FILEMODE=0444

# install rules
$(INSDIR1)/% $(INSDIR2)/%: %
	$(INS.file)

.KEEP_STATE:

all clean clobber cstyle depend lint lintlib strip:
	@true

install:        $(INSDIR1) $(INSDIR2) $(INSTALLABLES)
	@true

$(INSDIR1) $(INSDIR2):
	-$(INS.dir)

sccs:
		$(SCCSGET) $(SCCSGETFLAGS) $(FILES)

