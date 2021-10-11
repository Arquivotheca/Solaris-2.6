typedef struct action_item {
	char * label;
	void (*callback)();
	caddr_t data;
	Widget buttonWidget;
} ActionItem;

extern Widget CreateActionArea(Widget, ActionItem *, int);
