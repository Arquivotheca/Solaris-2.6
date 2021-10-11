/#include <stdio.h>/i\
#include "defs.h"\
#include "ui.h"

/bindtextdomain(".*"/s/"\."/SWM_LOCALE_DIR/

/^[ 	]*bindtextdomain(/a\
	/*\
	 * Work around for bugID 1107708\
	 */\
	init_usr_openwin();

/xv_main_loop/i\
	InitMain(argc, argv);
/XV_USE_LOCALE/a\
		XV_LOCALE_DIR, SWM_LOCALE_DIR, \
		XV_AUTO_CREATE, FALSE,
