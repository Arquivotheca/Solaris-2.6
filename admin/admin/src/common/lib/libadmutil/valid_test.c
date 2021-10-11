/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)valid_test.c	1.3	92/08/26 SMI"

/*
 * Interactive test driver for routines in valid.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include "valid.h"

#define MENU \
"1. Host IP Addr\n\
2. IP Netmask\n\
3. IP Network Number\n\
4. Host Ethernet Address\n\
5. Domain Name\n\
6. Hostname\n\
7. Timezone\n\
8. Group Name\n\
9. Group ID\n\
10. User Name\n\
11. Home Directory Path\n\
12. GCOS Field\n\
13. Integer\n\
14. Protocol Name\n\
15. Bootparams Key\n\
16. Group members\n\
17. Mail alias\n\
18. Network name\n\
19. Password\n"

#define	MENU_ITEMS	19

main()
{

	char buff[1024];
	int sel;
	int result;

	while (1) {
		printf("%s", MENU);
		printf("\nEnter selection: ");
		gets(buff);
		sel = atoi(buff);
		if ((sel < 1) || (sel > MENU_ITEMS)) {
			printf("Invalid selection\n");
			continue;
		}
		printf("Enter string to validate: ");
		gets(buff);
		switch (sel) {
		case 1:
			result = valid_host_ip_addr(buff);
			break;
		case 2:
			result = valid_ip_netmask(buff);
			break;
		case 3:
			result = valid_ip_netnum(buff);
			break;
		case 4:
			result = valid_host_ether_addr(buff);
			break;
		case 5:
			result = valid_domainname(buff);
			break;
		case 6:
			result = valid_hostname(buff);
			break;
		case 7:
			result = valid_timezone(buff);
			break;
		case 8:
			result = valid_gname(buff);
			break;
		case 9:
			result = valid_gid(buff);
			break;
		case 10:
			result = valid_uname(buff);
			break;
		case 11:
			result = valid_home_path(buff);
			break;
		case 12:
			result = valid_gcos(buff);
			break;
		case 13:
			result = valid_int(buff);
			break;
		case 14:
			result = valid_proto_name(buff);
			break;
		case 15:
			result = valid_bootparams_key(buff);
			break;
		case 16:
			result = valid_group_members(buff);
			break;
		case 17:
			result = valid_mail_alias(buff);
			break;
		case 18:
			result = valid_netname(buff);
			break;
		case 19:
			result = valid_passwd(buff);
			break;
		default:
			break;
		}
		printf("Result = %d\n\n", result);
	}
}
