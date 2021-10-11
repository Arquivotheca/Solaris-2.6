#ident "@(#)setdefaultkb.sh 1.1 93/11/24"
#
# This script may be used during profiled installs to specify
# the keyboard to be used during installation (and that is to
# become the default keyboard on subsequent reboots). It takes
# a single argument, which is the full path-name of the keyboard 
# mapfile that will be passed to the pcmapkeys command.  
#

if [ $# -ne 1 ]
then
	exit 1
else
	pcmapkeys $1
	echo $1 > /etc/defaultkb
	exit 0
fi
