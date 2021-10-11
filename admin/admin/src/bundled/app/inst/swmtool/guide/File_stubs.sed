/[ 	][ 	]*fprintf/i\
#ifdef XV_DEBUG
/[ 	][ 	]*fprintf/a\
#endif /* XV_DEBUG */
/[ 	][ 	]*fputs/i\
#ifdef XV_DEBUG
/[ 	][ 	]*fputs/a\
#endif /* XV_DEBUG */

/#include "swmtool.h"/a\
#include "defs.h"\
#include "ui.h"
