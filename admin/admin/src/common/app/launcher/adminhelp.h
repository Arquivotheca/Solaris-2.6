/* the following three defines are used for second argument to adminhelp */
#define TOPIC	'C'
#define HOWTO	'P'
#define REFER	'R'

#ifdef _cpluscplus
extern "C" {
#endif

int adminhelp(Widget, char, char*);

#ifdef _cpluscplus
}
#endif
