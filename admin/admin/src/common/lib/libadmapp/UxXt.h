/*---------------------------------------------------------------------
 * $Date: 93/03/14 18:53:26 $             $Revision: 2.10.2.2 $
 *---------------------------------------------------------------------
 * 
 *
 *             Copyright (c) 1991, Visual Edge Software Ltd.
 *
 * ALL  RIGHTS  RESERVED.  Permission  to  use,  copy,  modify,  and
 * distribute  this  software  and its documentation for any purpose
 * and  without  fee  is  hereby  granted,  provided  that the above
 * copyright  notice  appear  in  all  copies  and  that  both  that
 * copyright  notice and this permission notice appear in supporting
 * documentation,  and that  the name of Visual Edge Software not be
 * used  in advertising  or publicity  pertaining to distribution of
 * the software without specific, written prior permission. The year
 * included in the notice is the year of the creation of the work.
 *-------------------------------------------------------------------*/

/*****************************************************************************/
/*				UxXt.h				             */
/*****************************************************************************/

#ifndef	_UX_XT_H_
#define	_UX_XT_H_

#include <Xm/Xm.h>

#ifdef UIL_CODE
#include <Mrm/MrmPublic.h>
#endif /* UIL_CODE */

/*
#ifdef __STDC__
typedef char *caddr_t;
#endif
*/


/* The following macros are used in converting string values to the form
   required by the widgets */

#define	RES_CONVERT( res_name, res_value) \
	XtVaTypedArg, (res_name), XmRString, (res_value), strlen(res_value) + 1

#define	UxPutStrRes( wgt, res_name, res_value ) \
	XtVaSetValues( wgt, RES_CONVERT( res_name, res_value ), NULL )


#ifndef UX_INTERPRETER	/* Omit this section when interpreting the code */

/* The following macros are supplied for compatibility with swidget code */
#define	swidget			Widget
#define	UxWidgetToSwidget(w)	(w)
#define	UxGetWidget(sw)		(sw)
#define	UxIsValidSwidget(sw)	((sw) != NULL)
#define NO_PARENT             	((Widget) NULL)
#define UxThisWidget		(UxWidget)

/* Macros needed for the method support code */
#define	UxMalloc(a)		(malloc(a))
#define	UxRealloc(a,b)		(realloc((a), (b)))
#define	UxCalloc(a,b)		(calloc((a), (b)))
#define UxStrEqual(a,b)		(!strcmp((a),(b)))
#define UxGetParent(a)		(XtParent((a)))

#define	no_grab			XtGrabNone
#define	nonexclusive_grab	XtGrabNonexclusive
#define	exclusive_grab		XtGrabExclusive


/* The following global variables are defined in the main() function */
extern  XtAppContext	UxAppContext;
extern  Widget		UxTopLevel;
extern  Display		*UxDisplay;
extern  int		UxScreen;


/* The following are error codes returned by the functions in UxXt.c */
#define UX_ERROR           -1
#define UX_NO_ERROR        0

#ifdef UIL_CODE
#ifdef _NO_PROTO
extern	void    	UxMrmFetchError();
extern	MrmHierarchy    UxMrmOpenHierarchy();
extern	void    	UxMrmRegisterClass();
#else
extern	void    	UxMrmFetchError(MrmHierarchy, char *, Widget, Cardinal);
extern	MrmHierarchy    UxMrmOpenHierarchy( char *);
extern	void    	UxMrmRegisterClass( char *, Widget (*)(Widget, String, Arg *, Cardinal));
#endif /* _NO_PROTO */
#endif /* UIL_CODE */



/* The following are declarations of the functions in UxXt.c */

#ifdef _NO_PROTO

extern  int		UxPopupInterface();
extern  int		UxPopdownInterface();
extern  int		UxDestroyInterface();
extern  int		UxPutContext();
extern  caddr_t		UxGetContext();
extern  void		UxFreeClientDataCB();
extern  void		UxLoadResources();
extern  XmFontList	UxConvertFontList();
extern  void            UxDestroyContextCB();
extern	void    	UxDeleteContextCB();
extern	XtArgVal	UxRemoveValueFromArgList();
extern	Widget		UxChildSite();
extern	void*		UxNewContext ();

#else

#ifdef __cplusplus
extern "C" {
#endif

extern  int		UxPopupInterface( Widget wgt, XtGrabKind grab_flag );
extern  int		UxPopdownInterface( Widget wgt );
extern  int		UxDestroyInterface( Widget wgt);
extern  int		UxPutContext( Widget wgt, caddr_t context );
extern  caddr_t		UxGetContext( Widget wgt );
extern  void		UxFreeClientDataCB( Widget wgt, XtPointer client_data,
						 XtPointer call_data );
extern  void		UxLoadResources( char *fname );
extern  XmFontList	UxConvertFontList( char *fontlist_str );
extern  void            UxDestroyContextCB(Widget, XtPointer, XtPointer);
extern	void    	UxDeleteContextCB( Widget, XtPointer, XtPointer);
extern	XtArgVal	UxRemoveValueFromArgList( Arg *args,
						Cardinal *ptr_num_args,
						String res_name );
extern	Widget		UxChildSite( Widget );
extern	void*		UxNewContext (size_t size, int isSubclass);

#ifdef __cplusplus
}
#endif

#endif /* _NO_PROTO */

#endif /* ! UX_INTERPRETER */

#endif /* ! _UX_XT_H_ */

