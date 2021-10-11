#
# @(#)Makefile 1.4 95/02/01
#
#	SUNWvolg pkg Makefile

PKGARCH:sh=     /usr/bin/uname -a | /usr/bin/cut -d" " -f6
WD:sh=		pwd

all:		install

install:	pkgmk

pkgmk:
	@[ -d ${WD}/package ] || mkdir ${WD}/package
	@rm -rf ${WD}/package/SUNWvolg
	pkgmk -o -a ${PKGARCH} -d${WD}/package -f${WD}/prototype SUNWvolg
	/usr/sbin/pkgchk -d ${WD}/package SUNWvolg

clean clobber:
	rm -rf ${WD}/package copyright
