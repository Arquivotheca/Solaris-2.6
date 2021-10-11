#include <stdio.h>
#include "launcher_api.h"

SolsticeApp solstice_apps[] = {
	{
	  "Alarm Manager", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/alarm_mgr", 
	  "-x -y -z"},
	{
	  "Log Manager", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/log_mgr", 
	  "-a -b -c"},
	{
	  "Request Designer", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/reqDesigner_mgr", 
	  "-k -l -m"},
	{
	  "Node Manager", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/nodemgr", 
	  "-x -y -z"},
	{
	  "Host Manager", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/hostmgr", 
	  "-x -y -z"},
	{
	  "User Manager", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/usermgr", 
	  "-x -y -z"},
	{
	  "New Console", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/newconsole", 
	  "-x -y -z"},
	{
	  "Foo Manager", 
	  "/home/rgordon/icons/test.pm",
	  "/home/rgordon/bin/foomgr", 
	  "-x -y -z"},
};


int
solstice_get_apps(SolsticeApp ** s_apps, char * registry)
{
	if (registry == NULL) {
		*s_apps = solstice_apps;
		return(8);
	}
	else {
		return(0);
	}
}

int
solstice_del_app(const char * name, const char * registry)
{
	printf("deleting %s from registry %s\n",
			name, registry ? registry : ": default");
	if (strcmp(name, "Foo Manager") == 0)
		return(LAUNCH_LOCKED);
	return(0);
}

int
solstice_add_app(const SolsticeApp * app, const char * registry)
{
	printf("adding %d to registry %s\n",
			app->name, registry ? registry : ": default");
	printf("\tapp path : %s\n\targs : %s\t\nicon path: %s\n",
		app->app_path,
		app->app_args ? app->app_args : "none",
		app->icon_path ? app->icon_path : "none");
	if (strcmp(app->name, "Foo Manager") == 0)
		return(LAUNCH_DUP);
	return(0);
}
