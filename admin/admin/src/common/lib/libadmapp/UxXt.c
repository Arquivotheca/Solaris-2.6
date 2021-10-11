/*---------------------------------------------------------------------
 * $Date: 93/03/14 18:53:24 $             $Revision: 2.7.2.2 $
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
 /*------------------------------------------------------------------------
 *				UxXt.c                                    
 *-----------------------------------------------------------------------*/

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/X.h>
/* #include <X11/Xmd.h> */
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>

#ifdef UIL_CODE
#include <Mrm/MrmPublic.h>
#endif /* UIL_CODE */

#include "UxXt.h"

static	XContext	xcontext_id = 0;

/******************************************************************************
NAME:		GetTrueToplevel( wgt )

INPUT:		Widget	wgt		- the top user-created widget 

RETURN:		Widget			- the shell widget

DESCRIPTION:	Returns the true toplevel of that interface.
		This is needed for implicit shells where the widget passed to
		one of the Ux functions is not always the shell.

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
static	Widget 	GetTrueToplevel( wgt )
	Widget 	wgt;
#else
static	Widget 	GetTrueToplevel( Widget wgt )
#endif
{
	while (wgt && !XtIsShell(wgt)) 
	{
		wgt = XtParent(wgt);
	}
	return wgt;
}

/******************************************************************************
NAME:		handle_dialog_child( wgt, manage_func )

INPUT:		Widget	wgt			- the dialogShellWidget
		void	(*manage_func)()	- either XtManageChild
						  or XtUnmanageChild

RETURN:		int				- UX_NO_ERROR if successfull
						  UX_ERROR otherwise

DESCRIPTION:	Handles the popping up or popping down of dialog shells
		by managing or unmanaging their children.

CREATION:	Visual Edge Software		Sept 19/91   
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
static	int	handle_dialog_child( wgt, manage_func )
	Widget	wgt;
	void    (*manage_func)();
#else
static	int	handle_dialog_child( Widget wgt, void (*manage_func)(Widget) )
#endif
{
	int	i, num_children;
	Widget	*children;
	int	error_flag = UX_ERROR;

	XtVaGetValues( wgt,
		       XmNnumChildren, &num_children,
		       XmNchildren, &children,
		       NULL );

	/* We manage/unmanage the first rectObj child in the list.
	 * Note that the check for rectObjClass is necessary since some
	 * implementations of Motif add protocol children to the dialogShell.
	 */

	for (i = 0; i < num_children; i++)
	{
		if ( XtIsSubclass( children[i], rectObjClass ) )
		{
			(*manage_func)(children[i]);
			error_flag = UX_NO_ERROR;
		}
	}

	return ( error_flag );
}

/******************************************************************************
NAME:		popup_dialog( wgt, grab_flag )

INPUT:		Widget		wgt		- dialogShell to pop up
		XtGrabKind	grab_flag	- the grab flag

RETURN:		void

DESCRIPTION:	Pops up a dialogShell.

CREATION:	Visual Edge Software		Sept 19/91   
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
static	void	popup_dialog( wgt, grab_flag )
	Widget		wgt;
	XtGrabKind	grab_flag;
#else
static	void	popup_dialog( Widget wgt, XtGrabKind grab_flag )
#endif
{
	if ( handle_dialog_child( wgt, XtManageChild ) == UX_ERROR )
		XtPopup( wgt, grab_flag );
}

/******************************************************************************
NAME:		UxPopupInterface( wgt, grab_flag )

INPUT:		Widget		wgt		- Widget to popup
		XtGrabKind	grab_flag	- grab flag 

RETURN:		int			UX_ERROR or UX_NO_ERROR

DESCRIPTION:	Popups up an interface. The widget should be a toplevel widget.
		Note that special handling is required for dialogShells since
		those are popped up by managing their children if they have
		some.
		The grab_flag could be any of:
				no_grab	(XtGrabNone)
				nonexclusive_grab (XtGrabNonexclusive)
				exclusive_grab (XtGrabExclusive)

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
int	UxPopupInterface( wgt, grab_flag )
	Widget		wgt;
	XtGrabKind	grab_flag;
#else
int	UxPopupInterface( Widget wgt, XtGrabKind grab_flag )
#endif
{
	if (!(wgt = GetTrueToplevel(wgt)))
		return ( UX_ERROR );

	if ( XtIsSubclass( wgt, xmDialogShellWidgetClass ) )
	{
		popup_dialog( wgt, grab_flag );
	}
	else
	{
		XtPopup( wgt, grab_flag );
	}

	return ( UX_NO_ERROR );
}

/******************************************************************************
NAME:		popdown_dialog( wgt )

INPUT:		Widget	wgt		- dialogShell to popdown

RETURN:		void

DESCRIPTION:	Pops down a dialogShell.

CREATION:	Visual Edge Software		Sept 19/91   
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
static	void	popdown_dialog( wgt )
	Widget	wgt;
#else
static	void	popdown_dialog( Widget wgt )
#endif
{
	if ( handle_dialog_child( wgt, XtUnmanageChild ) == UX_ERROR )
		XtPopdown( wgt );
}

/******************************************************************************
NAME:		UxPopdownInterface( wgt )

INPUT:		Widget	wgt		- Widget to popdown

RETURN:		int			UX_ERROR / UX_NO_ERROR

DESCRIPTION:	Pops down an interface. The widget should be a toplevel widget.
		Note that special handling is required for dialogShells since
		those are popped down by unmanaging their children if they have
		some.

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
int	UxPopdownInterface( wgt )
	Widget	wgt;
#else
int	UxPopdownInterface( Widget wgt )
#endif
{
	if (!(wgt = GetTrueToplevel(wgt)))
		return ( UX_ERROR );

	if ( XtIsSubclass( wgt, xmDialogShellWidgetClass ) )
	{
		popdown_dialog( wgt );
	}
	else
	{
		XtPopdown( wgt );
	}

	return ( UX_NO_ERROR );
}

/******************************************************************************
NAME:		UxDestroyInterface( wgt )

INPUT:		Widget	wgt		- Widget to destroy

RETURN:		int			UX_ERROR / UX_NO_ERROR

DESCRIPTION:	Destroys an interface.  The widget should be a toplevel widget.
		
CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
int	UxDestroyInterface( wgt )
	Widget	wgt;
#else
int	UxDestroyInterface( Widget wgt )
#endif
{
	if (!(wgt = GetTrueToplevel(wgt)))
		return ( UX_ERROR );

	XtDestroyWidget (wgt);

	return ( UX_NO_ERROR );
}

/******************************************************************************
NAME:		UxDeleteContextCB( wgt, client_data, call_data )

INPUT:		Widget		wgt		- widget causing the callback
		XtPointer       client_data	- not used
		XtPointer	call_data	- not used

RETURN:		void

DESCRIPTION:	Deletes the X context entry.

EXT REFERENCES:	UxTopLevel, xcontext_id

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
void    UxDeleteContextCB( wgt, client_data, _call_data )
	Widget          wgt;
	XtPointer       client_data, _call_data;
#else
void    UxDeleteContextCB( Widget wgt, XtPointer client_data,
						XtPointer _call_data )
#endif
{
	XtPointer	call_data = _call_data;
	
	(void) XDeleteContext( XtDisplay( UxTopLevel ),
			       (Window) wgt,
			       (XContext) client_data );
}

/******************************************************************************
NAME:		UxPutContext( wgt, context )

INPUT:		Widget	wgt		- Widget
		XtPointer context	- context pointer

RETURN:		int			UX_ERROR / UX_NO_ERROR

DESCRIPTION:	Uses the X Context manager to store the given context pointer
		in a memory location that is indexed by the given widget id.
		Also adds a destroyCallback to delete that context when the
		widget is destroyed.

EXT REFERENCES:	UxTopLevel, xcontext_id
EXT EFFECTS:	xcontext_id

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
int	UxPutContext( wgt, context )
	Widget		wgt;
	caddr_t		context;
#else
int	UxPutContext( Widget wgt, caddr_t context )
#endif
{
	int		status;

	if ( xcontext_id == 0 )
		xcontext_id = XUniqueContext();

	if ( wgt == NULL )
		return ( UX_ERROR );

	status = XSaveContext( XtDisplay( UxTopLevel ), 
			       (Window) wgt, 
			       xcontext_id, 
			       (char *) context );
	if ( status != 0 )
		return ( UX_ERROR );

	XtAddCallback (wgt, XmNdestroyCallback, 
			UxDeleteContextCB, (XtPointer) xcontext_id);

	return ( UX_NO_ERROR );
}

/******************************************************************************
NAME:		UxGetContext( wgt )

INPUT:		Widget	wgt		- widget

RETURN:		caddr_t			- the context pointer

DESCRIPTION:	Uses the X Context manager to find the context pointer
		stored in a memory location indexed by the given widget id.

EXT REFERENCES:	UxTopLevel, xcontext_id

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
caddr_t		UxGetContext( wgt )
		Widget	wgt;
#else
caddr_t		UxGetContext( Widget wgt )
#endif
{
	int		status;
	caddr_t		context;

	if ( wgt == NULL )
		return ( (caddr_t) NULL );

	status = XFindContext( XtDisplay( UxTopLevel ), 
			       (Window) wgt, 
			       xcontext_id, 
			       &context );

	if ( status != 0 )
		return	( (caddr_t) NULL );

	return ( context );
}

/******************************************************************************
NAME:		UxDestroyContextCB ( wgt, client_data, call_data )

INPUT:		Widget		wgt		- widget
		XtPointer       client_data	- pointer to be freed
		XtPointer	call_data	- not used

RETURN:		void

DESCRIPTION:	This callback function simply frees the client data.

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
void    UxDestroyContextCB( _wgt, client_data, _call_data )
	Widget          _wgt;
	XtPointer       client_data, _call_data;
#else
void    UxDestroyContextCB( Widget _wgt, XtPointer client_data,
				XtPointer _call_data )
#endif
{
	Widget		wgt = _wgt;
	XtPointer	call_data = _call_data;

	if (client_data != NULL)
		XtFree((char *) client_data);
}
/******************************************************************************
NAME:		UxFreeClientDataCB( wgt, client_data, call_data )

INPUT:		Widget		wgt		- widget
		XtPointer       client_data	- pointer to be freed
		XtPointer	call_data	- not used

RETURN:		void

DESCRIPTION:	This callback function simply frees the client data.

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
void    UxFreeClientDataCB( _wgt, client_data, _call_data )
	Widget          _wgt;
	XtPointer       client_data, _call_data;
#else
void    UxFreeClientDataCB( Widget _wgt, XtPointer client_data,
				XtPointer _call_data )
#endif
{
	Widget		wgt = _wgt;
	XtPointer	call_data = _call_data;

	if (client_data != NULL)
		XtFree((char *) client_data);
}

/******************************************************************************
NAME:		UxLoadResources( fname )

INPUT:		char	*fname          - name of resource file

RETURN:		void

DESCRIPTION:	This function is provided as a stub function to load the
		resource file that was generated by the C Code Generator
		into the current toolkit resource database.
		A call to this function is automatically generated with the
		appropriate resource filename when resources are set to Public.

CREATION:	Visual Edge Software		April 6 1991
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
void    UxLoadResources( _fname )
	char	*_fname;
#else
void    UxLoadResources( char *_fname )
#endif
{
	char *fname = _fname;
}

/******************************************************************************
NAME:		UxConvertFontList( fontlist_str )

INPUT:		char	*fontlist_str		- string form of fontlist

RETURN:		XmFontList			- fontlist

DESCRIPTION:	Converts a fontlist from the resource-file string format
		to the XmFontList type expected in C code.
		The conversion is done by a call to the built-in Motif convertor
		and the return value points into Xt's resource cache
		and so the return value should NOT be XmFontListFree'd.

CREATION:	Visual Edge Software		June 17 1992
-----------------------------------------------------------------------------*/
#ifdef _NO_PROTO
XmFontList    UxConvertFontList( fontlist_str )
	char	*fontlist_str;
#else
XmFontList    UxConvertFontList( char *fontlist_str )
#endif
{
	XrmValue	from, to;
	XmFontList	fontlist = NULL;
	Boolean		status;

	from.size = strlen( fontlist_str ) + 1;
	from.addr = fontlist_str;

	to.size = sizeof(XmFontList);
	to.addr = (caddr_t) &fontlist;

	status = XtConvertAndStore( UxTopLevel,
				    XmRString, &from,
				    XmRFontList, &to );

	return ( fontlist );
}

/***************************************************************************
NAME:		UxRemoveValueFromArgList( args, ptr_num_args, res_name )
INPUT:		Arg		*args		: array of Arg's
		String		res_name	: name of resource
INPUT & OUTPUT:	Cardinal	*ptr_num_args	: number of Arg's
RETURN:		XtArgVal			: value of resource
DESCRIPTION:	Searches for the named resource in the array of Arg's.
		If found, the corresponding element is removed from the array,
		the number of Arg's is decremented, and the resource value
		is returned.
		If the named resource is not found, the return value is zero.
EXT REFERENCES: ---
EXT EFFECTS:	---
CREATION:	Nov 27/92

---------------------------------------------------------------------------*/
#ifdef _NO_PROTO
XtArgVal	UxRemoveValueFromArgList( args, ptr_num_args, res_name )
	Arg		*args;
	Cardinal	*ptr_num_args;
	String		res_name;
#else
XtArgVal	UxRemoveValueFromArgList( Arg *args, Cardinal *ptr_num_args,
						String res_name )
#endif
{
	Cardinal	i, num_args;

	num_args = *ptr_num_args;

	for (i = 0; i < num_args; i++)
	{
		if ( ! strcmp( args[i].name, res_name ) )
		{
			Cardinal	j;
			XtArgVal	res_value = args[i].value;

			/* We remove this arg from the array by moving
			   the remaining args down one in the array
			   and decreasng the arg count */

			--num_args;
			for (j = i; j < num_args; j++)
				args[j] = args[j + 1];

			*ptr_num_args = num_args;
			return ( res_value );
		}
	}

	return ( (XtArgVal) 0 );
}

/*------------------------------------------------------------------------
 * NAME:	UxNewContext
 *	<Allocate a context structure>
 * INPUT:	size 	-- of the desired structure
 *		isSubclass	-- nonzero if the requester is a subclass,
 *				   requiring that the result also be
 *				   returned for the base.
 * RETURNS:	Pointer to a data area of (at least) the desired size.
 *------------------------------------------------------------------------*/
#ifdef _NO_PROTO
void*	UxNewContext (size, isSubclass)
	size_t size;
	int    isSubclass;
#else
void*	UxNewContext (size_t size, int isSubclass)
#endif /* _NO_PROTO */
{
	static void* LastSubclassResult = 0;
	static int   LastResultSize = 0;

	void * result;

	if (LastSubclassResult) {
		result = LastSubclassResult;
	} else {
		result = XtMalloc(size);
	}

	if (isSubclass) {
		LastSubclassResult = result;
		if (LastResultSize  < size) {
			LastResultSize   = size;
		}
	} else {
		LastSubclassResult = 0;
		LastResultSize     = 0;
	}

	return (result);
}
/**  end of file ***/
