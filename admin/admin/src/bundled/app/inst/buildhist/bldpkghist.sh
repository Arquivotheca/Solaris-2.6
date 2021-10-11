#!/bin/sh
#ident   @(#)bldpkghist.sh 1.9 96/07/29 SMI
#
# arg 1 : directory of old package maps
#    <directory> is of form:
#
#             <directory>
#		  |
#            --------------------
#	    |         |        |
#	SUNWcsr    SUNWcar.m  SUNWadmr   ......
#           |          |        |
#        pkgmap      pkgmap    pkgmap
#
# arg 2 : directory containing new packages (flat directory structure,
#         similar to that of the directory of the old package maps,
#         but these must the full packages).
# arg 3 : pathname of old history file
# arg 4 : directory containing any consolidation-provided "special"
#         new pkghistory entries (for changes that can't be detected
#	  automatically, such as package renames).  The entries are
#	  names <package_name>.ph .  A "-" supplied for this argument
#         means that there are no "special" entries to be provided (an
#         empty directory will have the same result).
# arg 5 : file containing list of internal packages which should not
#	  be examined for new history entries ("-" if none).
# arg 6 : output location for final hist file
#

opd=$1
npd=$2
oldhistfile=$3
newhistdir=$4
excludefile=$5
finalhistfile=$6

#
# FIRST:  build file of components removed from the packages.
#

rm -f /tmp/*.$$

if [ "$excludefile" != "-" ] ; then
	excludelist=`cat $excludefile`
fi

echo Building list of packages:
(cd $npd;
for j in */pkginfo; do
	pkgname=`dirname $j`
	if [ "$excludelist" != "" ] ; then
		for i in $excludelist ; do
			if [ $i = $pkgname ] ; then
				pkgname=""
				break;
			fi
		done
	fi
	if [ "$pkgname" != "" ] ; then
		echo $pkgname
	fi
done ) > /tmp/pkglist.$$

echo Building removed file list:
cat > /tmp/cmp.sed.$$ << EOF
/ i /d
/^:/d
s/^1 . [^ ]* \([^ =]*\)[ =].*$/\1/
EOF

cat /tmp/pkglist.$$ | while read i ; do
	if [ -d $opd/$i ] ; then
		# generate list of files in old package, resolving macros
		echo $i
		if grep '\$' $opd/$i/pkgmap >/dev/null 2>&1 ; then
			(rm -f /tmp/pkgmacros.$$ /tmp/resolvedfiles.$$
			cat $opd/$i/pkginfo |
			while read j
			do
				echo `echo $j| sed -e 's/^\(.*\)=.*/\1/'`=\"`echo $j|sed -e 's/^\(.*\=\)//'`\" | grep -v '^PATH' >> /tmp/pkgmacros.$$
			done
			. /tmp/pkgmacros.$$
			sed -f /tmp/cmp.sed.$$ $opd/$i/pkgmap | 
		    	( while read j
		    	do
				eval echo $j 
		    	done ) | sort > /tmp/omap.$$
			)
		else
			sed -f /tmp/cmp.sed.$$ $opd/$i/pkgmap | sort > \
			    /tmp/omap.$$
		fi

		# generate file of PKG-ARCH-VERSION info for new pkgs
		grep '^PKG=' $npd/$i/pkginfo >> /tmp/newpkgs.$$
		grep '^ARCH=' $npd/$i/pkginfo >> /tmp/newpkgs.$$
		grep '^VERSION=' $npd/$i/pkginfo >> /tmp/newpkgs.$$

		# generate list of files in new package, resolving macros
		if grep '\$' $npd/$i/pkgmap >/dev/null 2>&1 ; then
			(rm -f /tmp/pkgmacros.$$ /tmp/resolvedfiles.$$
			cat $npd/$i/pkginfo |
			while read j
			do
				echo `echo $j| sed -e 's/^\(.*\)=.*/\1/'`=\"`echo $j|sed -e 's/^\(.*\=\)//'`\" | grep -v '^PATH' >> /tmp/pkgmacros.$$
			done
			. /tmp/pkgmacros.$$
			sed -f /tmp/cmp.sed.$$ $npd/$i/pkgmap | 
		    	( while read j
		    	do
				eval echo $j 
		    	done ) | sort > /tmp/nmap.$$
			)
		else
			sed -f /tmp/cmp.sed.$$ $npd/$i/pkgmap | sort > \
			    /tmp/nmap.$$
		fi

		# find files that are in old package, but not in new
		comm -23 /tmp/omap.$$ /tmp/nmap.$$  | sort -r > /tmp/rmfiles.$$
		(if [ -s /tmp/rmfiles.$$ ] ; then
			grep '^PKG=' $npd/$i/pkginfo
			grep '^ARCH=' $npd/$i/pkginfo
			grep '^VERSION=' $npd/$i/pkginfo
			sed 's/^./REMOVED_FILES=&/' /tmp/rmfiles.$$
		fi) >> /tmp/all_rmfiles.$$
	fi
done


#
# Build concatenated file of new history entries
#

touch /tmp/newhistfile.$$
anyfound=0
origdir=`pwd`
if [ "$newhistdir" != "-" ] ; then
	cd $newhistdir
	if ls *.ph >/dev/null 2>&1 ; then
	    for j in *.ph ; do
		#
		# VERSION can be:
		#	xxx
		#	xxx:yyy
		#	latest
		#	xxx:latest
		#
		# Where the string 'latest' will be replaced by the
		# latest version
		#
		grep '^VERSION=.*latest' $j >/dev/null 2>&1
		if [ $? = 0 ] ; then
			grep '^VERSION=latest.*:' $j >/dev/null 2>&1
			if [ $? = 0 ] ; then
				echo "Error: $j: bad version"
				exit 99
			fi
			pkgname=`basename $j ".ph"`
			version=`grep "^VERSION=" $npd/$pkgname/pkginfo`
			version=`expr $version : 'VERSION=\(.*\)'`
			sed "/^VERSION/s/latest/$version/" $j \
			   >> /tmp/newhistfile.$$
		else
			cat $j >> /tmp/newhistfile.$$
		fi
	    done
	fi
	cd $origdir
fi

#
# combine them all into final history file
#

echo Building final history file
#
# If the removed file list is empty, don't send in the -r argument.
#
cmdline="./buildhist -p $oldhistfile -n /tmp/newhistfile.$$ \
	-v /tmp/newpkgs.$$ -o $finalhistfile"
if [ -f /tmp/all_rmfiles.$$ ]; then
	cmdline="$cmdline -r /tmp/all_rmfiles.$$"
fi
$cmdline
status=$?
rm -f /tmp/*.$$
rm -fr /tmp/hist.concat.$$
exit $status
