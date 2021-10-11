#
#   Copyright(c) 1995 Sun Microsystems Inc.
#   All rights reserved.
#

#       @(#)Makefile	1.6     10/15/96   SunSoft Development

# Makefile for miscellaneous files
#
# FILES are those things that are just used as is

include ../Makefile.cmd

MSG_DOMAIN=SUNW_PATCH_SCRIPTS

SCRIPTS= patchadd patchrm

all:	${SCRIPTS} msgs $(ROOTUSRSBINPROG)

install:	all

msgs:	${MSG_DOMAIN}.po

clean:

clobber:
	-rm -f ${SCRIPTS} *.po *.mo core

${MSG_DOMAIN}.po: ${SCRIPTS}
	@$(ROOTADMINBIN)/xgetsh -d $(MSG_DOMAIN) ${SCRIPTS}
	@chmod 755 $@
	@rm -f $(ROOTUSRLIBMSGS)/$@
	@cp $@ $(ROOTUSRLIBMSGS)

%:	%.ksh
	@rm -f $@
	@cp $< $@
	@chmod 555 $@
	@rm -f $(ROOT)/usr/sbin/$@
	@cp $@ $(ROOT)/usr/sbin

clean_msgs:
	@rm -f $(SCRIPTS).po

include ../Makefile.targ
