#!/bin/sh
echo `gettext SUNW_BLOT_BLOB "Trailing double \"`
echo `gettext SUNW_BLOT_BLOB 'Traling double in single \"`
echo `gettext SUNW_BLOT_BLOB "Traling quoted \"test\"`

echo `gettext SUNW_BLOT_BLOB "Copy \"test\" was no OK."`
echo `gettext SUNW_BLOT_BLOB "Copy \"test\""`
echo `gettext SUNW_BLOT_BLOB "\"test\""`
echo `gettext SUNW_BLOT_BLOB 'Copy "single test" was no OK.'`
echo `gettext SUNW_BLOT_BLOB 'Copy single \'test\' was no OK.'`
echo `gettext SUNW_BLOT_BLOB "Copy 'test' was no OK."`

echo `gettext SUNW_BLOT_BLOB "Next Line \"test\" was no OK \
next line."`
echo `gettext SUNW_BLOT_BLOB "Next Line \"test\"\
   next line."`
echo `gettext SUNW_BLOT_BLOB "Next Line \"test\"\
     next line."`
echo `gettext SUNW_BLOT_BLOB 'Next Line "single test" was no OK\
 next line.'`

echo `gettext SUNW_BLOT_BLOB "Traling quoted \"test\"`
echo `gettext SUNW_BLOT_BLOB "Trailing double \"`
echo `gettext SUNW_BLOT_BLOB 'Traling double in single \"`

echo `gettext SUNW_BLOT_BLOB "Next Line \"test\" was no OK \
next line.\
another line."`
echo `gettext SUNW_BLOT_BLOB "Next Line \"test\"\
   next line.\
   another line."`
echo `gettext SUNW_BLOT_BLOB "Next Line \"test\"\
     next line.\
     another line."`
echo `gettext SUNW_BLOT_BLOB 'Next Line "single test" was no OK\
 next line.\
 another line.'`
