/*
 *
 * exMess.c
 *
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <desktop/tt_c.h>


#define NUM_OPS 3

char buf[150];

char *cmds[NUM_OPS] = { "/etc/lp/alerts/jobdone",
						"/etc/lp/alerts/printer",
						"/etc/lp/alerts/form"  };
char *ops[NUM_OPS] = { "lp_job_done","lp_printer_alert","lp_form_alert"  };
char *files[NUM_OPS]= {"/etc/lp/printers", "/etc/lp/printers","/etc/lp/forms" };
int goodTally[NUM_OPS];
int badTally[NUM_OPS];
int goodTotal,badTotal;

/* jobdone parameters: "lp_job_done" "/etc/lp/printers" "$1" "$2" ""
   printer parameters: "lp_printer_alert" "/etc/lp/printers" "$1" "$3" "$2"
	form parameters:    "lp_form_alert" "/etc/lp/forms" "$1" "$3" "$2"
*/


static Tt_pattern     ourPattern;

/*
 * Initialize our ToolTalk environment.
 */
int
registerPattern(char *op,char *file)
{
   int   mark;
 
   mark = tt_mark();
 
	ourPattern = tt_pattern_create();
	tt_pattern_op_add(ourPattern, op);
	tt_default_file_set(file);
	tt_pattern_scope_add(ourPattern, TT_FILE);
	tt_pattern_file_add(ourPattern,file);
	tt_pattern_category_set(ourPattern, TT_OBSERVE);
	tt_file_join(file);
	if (tt_pattern_register(ourPattern) != TT_OK) {
		printf("could not register pattern %s %s\n",op,file);
		return 0;
		}
 
   /*
    * Note that without tt_mark() and tt_release(), the above
    * combination would leak storage -- tt_default_session() returns
    * a copy owned by the application, but since we don't assign the
    * pointer to a variable we could not free it explicitly.
    */
 
   tt_release(mark);
	return 1;
}

void
sendMsg(int opNum,char *p1, char *p2, char *p3) {
	int			ttfd;
	int		mark;
	Tt_message	msg;

	sprintf(buf,"PATH=\"/usr/openwin/bin:$PATH\"; export PATH;env >/tmp/fooTest; %s '%s' '%s' '%s' ",cmds[opNum],p1,p2,p3);
	system(buf);
}



/*
 * Handle any incoming ToolTalk messages.
 */
int
receiveMsg(int opNum,char *exP1, char *exP2, char *exP3) 
{
   Tt_message     msg_in;
   char        *file,*op,*p1,*p2,*p3;
	int   mark;
	char  *printerName,*date,*message;
	int cnt;
 
   msg_in = tt_message_receive();
	cnt = 0;
	while (msg_in == NULL) {
      fprintf(stderr,"Waiting for message.\n");
		sleep(1);
		msg_in = tt_message_receive();
		if ( cnt > 5 ) {
			return 0;
			}
		cnt++;
      };
   if (tt_pointer_error(msg_in) == TT_ERR_NOMP) {
      fprintf(stderr,"ToolTalk server down.\n");
      exit(0);
   }
			 
	mark = tt_mark();
	file = tt_message_file(msg_in);
	op = tt_message_op(msg_in);
	p1 = tt_message_arg_val(msg_in,0);
	p2 = tt_message_arg_val(msg_in,1);
	p3 = tt_message_arg_val(msg_in,2);
	if (strcmp(files[opNum],file) || strcmp(ops[opNum],op) || 
		 strcmp(p1,exP1) ||
		 strcmp(p2,exP2) || strcmp(p3,exP3))  {
		printf("***BAD op(%s,%s) file(%s,%s) p1(%s,%s) p2(%s,%s) (%s,%s)\n",
				ops[opNum],op,files[opNum],file,exP1,p1,exP2,p2,exP3,p3);
		badTally[opNum]++;
		badTotal++;
		}
	else  {
		goodTally[opNum]++;
		goodTotal++;
		}
	
	tt_release(mark);
	tt_message_reply(msg_in);
   tt_message_destroy(msg_in);
 
   return 1;
}


int
doOneMessage(int opNum,char *p1, char *p2, char *p3) {
   if ( opNum == 0 ) {
	   sendMsg(opNum,p1,p2,"");
	   return(receiveMsg(opNum,p1,p2,""));
	   }
   else {
	   sendMsg(opNum,p1,p3,p2); 
	   return(receiveMsg(opNum,p1,p2,p3));
		}
  }

#define NAMELENG 25

void
assignRandName(char *a)  {
	int i,len,r;
   len = (rand() % (NAMELENG-1));
	for ( i = 0 ; i <= len ; i++ ) {
		r = rand() % 62;
		a[i] = (r < 26 ? 'A' + r : ( r < 52 ? 'a' + r -26 : '0' + r -52 ));
		}
	a[len+1] = 0;
	}
main(int argc, char **argv)
{
   char  *procid = tt_open();
	int i,opNum,numTests;
	char p1[NAMELENG],p2[NAMELENG],p3[NAMELENG];
   int   ttfd;

   if (tt_pointer_error(procid) != TT_OK) {
      fprintf(stderr,"error in tt_open %d\n",procid);
      return 0;
   }  
	for ( i = 0 ; i < NUM_OPS ; i++ ) {
		if (registerPattern(ops[i],files[i]) == 0) exit(-1);
		}
   ttfd = tt_fd();
	numTests = (argc > 1 ? atoi(argv[1]) : 100);
	printf("run %d tests. (Specify arg for different number.)\n",numTests);


	for ( i = 0 ; i < numTests ; i++ ) {
		opNum = rand() % NUM_OPS;
		assignRandName(p1);
		assignRandName(p2);
		assignRandName(p3);
		if (doOneMessage(opNum,p1,p2,p3) == 0) {
		  printf("\nsomething went wrong with ToolTalk, exiting ...\n");
		  break;
		  }
		}
	printf("\nTotal tests run:      %4d\n",goodTotal+badTotal);
	printf("   Successful tests:  %4d\n",goodTotal);
	printf("   Failed tests:      %4d %s\n",badTotal,
		  (badTotal > 0 ? "******" : ""));

	printf("tests succeeded for:\n");
	for ( i = 0 ; i < NUM_OPS ; i++ ) {
		printf("   %-25s %d times\n",ops[i],goodTally[i]);
		}
	if ( badTotal > 0 ) {
		printf("***** tests failed for: *****\n");
		for ( i = 0 ; i < NUM_OPS ; i++ ) {
			if ( badTally[i] > 0 ) {
				printf("   %-25s %d times\n",ops[i],badTally[i]);
				}
			}
		}
   tt_close();
	exit(0);
}
