#
# @(#)Makefile 1.2 95/02/01
#
#	SUNW0volg pkg Makefile

PKGARCH:sh=     /usr/bin/uname -a | /usr/bin/cut -d" " -f6
WD:sh=		pwd

all:		install

install:	pkgmk

pkgmk:
	@[ -d ${WD}/package ] || mkdir ${WD}/package
	@rm -rf ${WD}/package/SUNW0volg
	pkgmk -o -a ${PKGARCH} -d${WD}/package -f${WD}/prototype SUNW0volg
	/usr/sbin/pkgchk -d ${WD}/package SUNW0volg

clean clobber:
	rm -rf ${WD}/package copyright
