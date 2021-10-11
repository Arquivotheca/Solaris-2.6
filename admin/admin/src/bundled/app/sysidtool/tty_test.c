#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "sysid_ui.h"
#include "tty_defs.h"
#include "sysid_msgs.h"

int	erasech;
int	killch;
int	cmdy;

void
main()
{
	char	*items[100];
	char	junk[256];
	int	i, y;

	(void) fprintf(stderr, "Ready? ");
	(void) gets(junk);

	(void) initscr();
	(void) cbreak();
	(void) noecho();
	(void) leaveok(stdscr, FALSE);
	(void) scrollok(stdscr, FALSE);
	(void) keypad(stdscr, TRUE);
	(void) typeahead(-1);

	cmdy = LINES - (NCMDLINES);

	erasech = erasechar();
	killch = killchar();
	
	i = 0;
	items[i++] = UNITED_STATES;
	items[i++] = AFRICA;
	items[i++] = WESTERN_ASIA;
	items[i++] = EASTERN_ASIA;
	items[i++] = AUSTRALIA_NEWZEALAND;
	items[i++] = CANADA;
	items[i++] = EUROPE;
	items[i++] = CENTRAL_AMERICA;
	items[i++] = SOUTH_AMERICA;
	items[i++] = GMT_OFFSET;
	items[i++] = TZ_FILE_NAME;
	items[i++] = EGYPT;
	items[i++] = LIBYA;
	items[i++] = TURKEY;
	items[i++] = WESTERN_USSR;
	items[i++] = IRAN;
	items[i++] = ISRAEL;
	items[i++] = SAUDI_ARABIA;
	items[i++] = CHINA;
	items[i++] = TAIWAN;
	items[i++] = HONGKONG;
	items[i++] = JAPAN;
	items[i++] = KOREA;
	items[i++] = SINGAPORE;
	items[i++] = TASMANIA;
	items[i++] = QUEENSLAND;
	items[i++] = NORTH;
	items[i++] = SOUTH;
	items[i++] = WEST;
	items[i++] = VICTORIA;
	items[i++] = NEW_SOUTH_WALES;
	items[i++] = BROKEN_HILL;
	items[i++] = YANCOWINNA;
	items[i++] = LHI;
	items[i++] = NEW_ZEALAND;
	items[i++] = NEWFOUNDLAND;
	items[i++] = ATLANTIC;
	items[i++] = EASTERN;
	items[i++] = CENTRAL;
	items[i++] = EAST_SASKATCHEWAN;
	items[i++] = MOUNTAIN;
	items[i++] = PACIFIC;
	items[i++] = YUKON;
	items[i++] = BRITAIN;
	items[i++] = EIRE;
	items[i++] = ICELAND;
	items[i++] = POLAND;
	items[i++] = WESTERN_EUROPE;
	items[i++] = MIDDLE_EUROPE;
	items[i++] = EASTERN_EUROPE;
	items[i++] = MEXICO_BAJA_NORTE;
	items[i++] = MEXICO_BAJA_SUR;
	items[i++] = MEXICO_GENERAL;
	items[i++] = CUBA;
	items[i++] = BRAZIL_EAST;
	items[i++] = BRAZIL_WEST;
	items[i++] = BRAZIL_ACRE;
	items[i++] = BRAZIL_DE_NORONHA;
	items[i++] = CHILE_CONTINENTAL;
	items[i++] = CHILE_EASTER_ISLAND;
	items[i++] = USA_EASTERN;
	items[i++] = USA_CENTRAL;
	items[i++] = USA_MOUNTAIN;
	items[i++] = USA_PACIFIC;
	items[i++] = USA_EAST_INDIANA;
	items[i++] = USA_ARIZONA;
	items[i++] = USA_MICHIGAN;
	items[i++] = USA_SAMOA;
	items[i++] = USA_ALASKA;
	items[i++] = USA_ALEUTIAN;
	items[i++] = USA_HAWAII;
	items[i++] = RETURN_TO_REGIONS_MENU;

	i = menu(MARGAIN_TOP, MARGAIN_LEFT,
		LINES - MARGAIN_TOP, COLS - 2 * MARGAIN_LEFT,
		(char *)0, PLEASE_SELECT_TIMEZONE, items, i, 0);
	(void) erase();
	(void) nocbreak();
	(void) echo();
	(void) move(stdscr->_maxy, 0);
	(void) refresh();
	(void) endwin();
	(void) fflush(stdout);
	(void) fflush(stderr);

	if (i == -1)
		(void) fprintf(stderr, "menu returned error\n");
	else
		(void) fprintf(stderr,
			"item %d (%s) is selected\n", i, items[i]);
	exit (0);
	/*NOTREACHED*/
}
