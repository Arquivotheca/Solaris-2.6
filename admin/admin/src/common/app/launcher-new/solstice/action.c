
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>

#include "action.h"
#include "util.h"	

#define TIGHTNESS 20

Widget
getActionButton(ActionItem * item, char * name)
{
}	

Widget
CreateActionArea(Widget parent, ActionItem * items, int n_items)
{
	Widget area, w;
	Widget sep;
	int i;
	
	area = XtVaCreateWidget("actionArea", 
		xmFormWidgetClass, parent,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 1,
		XmNskipAdjust, True,
#if 0
		XmNfractionBase, TIGHTNESS*n_items-1,
#else
		XmNfractionBase, 2*n_items+1,
#endif
	
		NULL);

        sep = XtVaCreateManagedWidget( "sep",
                        xmSeparatorWidgetClass,
                        area,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNleftOffset, 0,
                        XmNrightOffset, 0,
                        NULL );

	for (i=0; i < n_items; i++) {
		w = XtVaCreateManagedWidget(items[i].label,
			xmPushButtonWidgetClass, area,
#if 0
			XmNleftAttachment, i ? XmATTACH_POSITION:XmATTACH_FORM,
			XmNleftPosition, TIGHTNESS * i,
#else
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 2*i+1,
#endif
			XmNtopAttachment,  XmATTACH_FORM,
			XmNtopOffset, OFFSET,
#if 0
			XmNtopAttachment,  XmATTACH_WIDGET,
			XmNtopWidget, sep,
#endif
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, OFFSET,
#if 0
			XmNrightAttachment, 
			    i != n_items-1 ? XmATTACH_POSITION : XmATTACH_FORM,
			XmNrightPosition, TIGHTNESS*i + (TIGHTNESS-1),
#else
			XmNrightAttachment,  XmATTACH_POSITION,
			XmNrightPosition, 2*i + 2,
#endif
			XmNshowAsDefault, i == 0,
			XmNdefaultButtonShadowThickness, 1,
			XmNnavigationType, XmTAB_GROUP,
			NULL);

		if (items[i].callback)
			XtAddCallback(w, XmNactivateCallback,
				items[i].callback, items[i].data);
		if (i == 0) {
			Dimension height, h;
			XtVaGetValues(area, XmNmarginHeight, &h, NULL);
			XtVaGetValues(w, XmNheight, &height, NULL);
			height += 2 * h;
			XtVaSetValues(area,
			XmNdefaultButton, w,
			XmNpaneMaximum, height,
			XmNpaneMinimum, height,
			NULL);
		}
		items[i].buttonWidget = w;
	}		
	return(area);
}
