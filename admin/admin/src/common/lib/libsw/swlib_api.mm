.\" CUT HERE
.\"@(#)swlib_api.mm 1.2 95/02/24
.\" uses tbl and (doctools) troff mm macros
.ds HF 3 3 3 2 2 2 2
.ds HP 12 12 10 10 10 10 10
.nr Hs 3
.nr Hb 3
.tr ~
.de Cb \" Begin change bar region
.mc \s12\(br\s0
..
.de Ce \" End change bar region
.mc
..
.Cl 3
.PF "'\fISun Confidential'revision 1'Software Library API\fP'"
.ND "February 15, 1995"
.TL 
Installation Software Library API Description
.AU
.MT 4
.H 1 "Overview"
This appendix defines the new API to the installation software
library.
The first section defines the data structures.  The second
section defines the functions.
The header file which defines the data structures and the
function prototypes for the new software library API is in the
file swmgmt_api.h.  The old header sw_api.h header file will
still be available for programs using the old interface to
the software library.
.H 2 "Error Reporting"
All functions in the library use a uniform error reporting mechanism.
The functions that can complete with errors all return some value
which is effectively a boolean value that indicates success or failure
(it may also be a pointer).  In the case of functions that return 
pointers, a NULL return indicates a failure.  Functions that do not
return pointers, but which can complete with an error, return TRUE
for success and FALSE for failure.  In all cases, the function has
a argument which is of type
.DS
	SW_error_info **
.DE
When the function completes successfully, that pointer points to
NULL.  If the function completed in error, the pointer points to
a structure of type SW_error_info, which contains an error code,
and an optional data structure which contains more information
about the failure (a space table in the case of an insufficient
space error, for example).  The optional data structure containing
additional error information in the union \fIsp_specific_errinfo\fR.
The selection of the substructure in the union will depend on the
value of the error code.
.P
The reason for passing the error reporting structure as an argument
(instead of having some global errno-like variable and a global
error-information structure) is to enable the library to be made
MT-safe at some point.
.H 2 "Fault Injection Support"
The environment variable SW_INJECT_FAULT can set to an error
code (of the SW_return_code type) to induce the software library
to simulate the occurrence of the specified error.
.bp
.H 1 "Data Structures"
.DS
.so h.extract
.DE
.bp
.H 1 "Function Specifications"
.H 2 "list_available_services"
.P
NAME
.br
.in +.6i
list_available_services - list the services on an installation medium
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
SW_service_list *list_available_services(char *medium_path, SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIlist_available_services\fR lists the services that are available for
installation from the installation medium that is located at
\fImedium_path\fR.  \fImedium_path\fR is the location of the
root directory of the installation CD.
.P
The number of services in the returned structure is provided in the
\fIsv_svl_num_services\fR field of the \fISW_service_list\fR data
structure.  The individual services are named in the linked list
pointed to by the sw_svl_services field.  Each service identification
structure has four fields, reporting the OS name of the service,
the version of the OS, the instruction set architecture of the service,
and the platform or platform group supported by the service.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If successful, \fIlist_available_services\fR returns a pointer to a
\fISW_service_list\fR structure that contains the number and list of services.
If the function fails, it will return NULL.  In that case, *\fIerrinfo\fR
will point to
a SW_error_info data structure, which will contain the specific error code
and additional information, if necessary.
.in -.6i
.SP
.ne 5
.H 2 "list_installed_services"
.P
NAME
.br
.in +.6i
list_installed_services - list the services installed on a system.
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
SW_service_list *list_installed_services(SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIlist_installed_services\fR lists the services that are installed on
the local system.
.P
The number of services in the returned structure is provided in the
\fIsv_svl_num_services\fR field of the \fISW_service_list\fR data
structure.  The individual services are named in the linked list
pointed to by the sw_svl_services field.  Each service identification
structure has four fields, reporting the OS name of the service,
the version of the OS, the instruction set architecture of the service,
and the platform or platform group supported by the service.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If successful, \fIlist_installed_services\fR returns a pointer to a
\fISW_service_list\fR structure that contains the number and list of services.
If the function fails, it will return NULL.  In that case, *\fIerrinfo\fR
will point to
a SW_error_info data structure, which will contain the specific error code
and additional information, if necessary.
.in -.6i
.SP
.ne 5
.H 2 "list_avail_svc_platforms"
.P
NAME
.br
.in +.6i
list_avail_svc_platforms - list the individual platforms supported by an
available service.
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
SW_platform *list_avail_svc_platforms(char *medium_path, SW_service *service, SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIlist_avail_svc_platforms\fR lists the platforms that are supported by
the service named by \fIservice\fR on the medium at \fImedium_path\fR.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If successful, \fIlist_avail_svc_platforms\fR returns a pointer to a
linked list of \fISW_platform\fR structures, each of which names a
platform.  
If the function fails, it will return NULL.  In that case, *\fIerrinfo\fR
will point to
a SW_error_info data structure, which will contain the specific error code
and additional information, if necessary.
.in -.6i
.SP
.ne 5
.H 2 "list_installed_svc_platforms"
.P
NAME
.br
.in +.6i
list_installed_svc_platforms - list the individual platforms supported by an
installed service.
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
SW_platform *list_installed_svc_platforms(SW_service *service, SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIlist_installed_svc_platforms\fR lists the platforms that are supported by
the installed service named by \fIservice\fR.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If successful, \fIlist_installed_svc_platforms\fR returns a pointer to a
linked list of \fISW_platform\fR structures, each of which names a
platform.  
If the function fails, it will return NULL.  In that case, *\fIerrinfo\fR
will point to
a SW_error_info data structure, which will contain the specific error code
and additional information, if necessary.
.in -.6i
.SP
.ne 5
.H 2 "validate_service_modifications"
.P
NAME
.br
.in +.6i
validate_service_modifications - validate a list of proposed service modifications
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
int validate_service_modification(SW_service_modspec *modspec, SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIvalidate_service_modifications\fR takes a list of proposed service
modifications (adds, removes, and upgrades) and
verifies that the modifications will
succeed \fBif all done at once\fR.  The validation performs space checking and
dependency checking.  If the case of an SW_ADD_SERVICE operation, the
.I
sw_svmod_media
.R
field must be set to point to the installation medium containing the
service to be installed.  The
.I
sw_svmod_newservice
.R
field indicates the service to be added.  In the case of an SW_REMOVE_SERVICE
operation, the
.I
sw_svmod_oldservice
.R
field indicates the service to be removed.
In the case of an SW_UPGRADE_SERVICE operation, the
.I
sw_svmod_oldservice
.R
value identifies the service to be upgraded and the
.I
sw_svmod_newservice
.R
field indicates the service \fBto which\fR the service will be upgraded.
The
.I
sw_svmod_media
.R
field indicates the installation medium containing the service to which
the old service will be upgraded.
.P
The SW_SVMOD_UNCONDITIONAL bit in the 
.I
sw_svmodspec_flags
.R
flag word can be set
to indicate that the service operation should be performed
unconditionally.  If the SW_SVMOD_UNCONDITIONAL flag is set, dependency
errors will be ignored, but space errors will still prevent the action
from taking place.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If the validation indicates that the service modifications will succeed, it
returns a non-zero value.  If the validation fails, the function returns
zero and sets *\fIerrinfo\fR to point a SW_error_info data structure which
identifies the reason for failure and provides additional information
about the failure (such as file system space table in the case of an
SW_INSUFFICIENT_SPACE) error.  The possible errors are:
.VL 33 5
.LI SW_INSUFFICIENT_SPACE
There is insufficient disk space on the system to complete the set of
requested modifications.  The specific error information area will
contains a structure of the form
.I
sw_space_results
.R
which contains a table of file systems and the space needed in each.
.LI SW_DEPENDENCY_FAILURE
One of the services requested for removal or upgrade cannot be
removed because there are clients that currently use it.  The
specific error information is TBD.
.LI SW_MEDIA_FAILURE
The media containing the service to be added couldn't be accessed.
.LI SW_INVALID_SVC
One of the services listed was invalid.
.LI SW_INCONSISTENT_REV
One of the services couldn't be added because it was of the same
release and instruction-set-architecture as a service
already present on the system, but was of a different revision
(beta vs. FCS, for example).  The specific package whose version
differed is returned in the specific error information area in
a structure of the form
.I
sw_diffrev
.R
.LE
.in -.6i
.SP
.ne 5
.H 2 "execute_service_modifications"
.P
NAME
.br
.in +.6i
execute_service_modifications - execute a list of proposed service modifications
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
int execute_service_modification(SW_service_modspec *modspec, SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIexecute_service_modifications\fR takes a list of proposed service
modifications (adds, removes, and upgrades) and performs those 
modifications to the system.  The function does not return until
all of the system modifications are complete, or an error occurs.
.P
The SW_SVMOD_UNCONDITIONAL bit in the 
.I
sw_svmodspec_flags
.R
flag word can be set
to indicate that the service operation should be performed
unconditionally.  If the SW_SVMOD_UNCONDITIONAL flag is set, dependency
errors will be ignored, but space errors will still prevent the action
from taking place.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If the service modifications succeed, the function returns 
a non-zero value.  If the operation fails, the function returns
zero and sets *\fIerrinfo\fR to point a SW_error_info data structure which
identifies the reason for failure and provides additional information
about the failure.  The possible errors are:
.VL 33 5
.LI SW_INSUFFICIENT_SPACE
There is insufficient disk space on the system to complete the set of
requested modifications.  The specific error information area will
contains a structure of the form
.I
sw_space_results
.R
which contains a table of file systems and the space needed in each.
.LI SW_OUT_OF_SPACE
The system actually ran out of space while performing the requested
actions.  Typically, the validation should prevent this from happening,
but space failures might occur if some other process used up space
on the file system after the validation completed.  The specific error
information will contain a
.I
sw_out_of_space
.R
structure, which identifies the file system that ran out of space,
and also reports the new list of services on the system.
.LI SW_DEPENDENCY_FAILURE
One of the services requested for removal or upgrade cannot be
removed because there are clients that currently use it.  The
specific error information is TBD.
.LI SW_MEDIA_FAILURE
The media containing the service to be added couldn't be accessed.
.LI SW_INVALID_SVC
One of the services listed was invalid.
.LI SW_INCONSISTENT_REV
One of the services couldn't be added because it was of the same
release and instruction-set-architecture as a service
already present on the system, but was of a different revision
(beta vs. FCS, for example).  The specific package whose version
differed is returned in the specific error information area in
a structure of the form
.I
sw_diffrev
.R
.LE
.in -.6i
.SP
.ne 5
.H 2 "get_createroot_info"
.P
NAME
.br
.in +.6i
get_createroot_info - get the information needed to set up a diskless
client root file system.
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
SW_createroot_info *get_createroot_info(SW_service *service, SW_error_info **errinfo);
.in -.6i
.SP
DESCRIPTION
.in +.6i
\fIget_createroot_info\fR returns the list of packages that need to be
added and the remote mounts that need to be set up in order to create a
diskless client root file system of the installed service named
by \fIservice\fR.  The function also returns the total size of the package
components.
.in -.6i
.SP
RETURN VALUES
.in +.6i
If successful, \fIget_createroot_info\fR returns a pointer to
a \fISW_createroot_info\fR structure, which lists the packages to be added
and the remote mounts to be set up.
If the function fails, it will return NULL.  In that case, *\fIerrinfo\fR
will point to
a SW_error_info data structure, which will contain the specific error code
and additional information, if necessary.
.in -.6i
.SP
.ne 5
.H 2 "free_service_list"
.P
NAME
.br
.in +.6i
free_service_list - free a service list
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
void free_service_list(SW_service_list *service_list);
.in -.6i
.SP
DESCRIPTION
.in +.6i
Free a service list structure.
.in -.6i
.SP
RETURN VALUES
.in +.6i
none.
.in -.6i
.SP
.ne 5
.H 2 "free_platform_list"
.P
NAME
.br
.in +.6i
free_platform_list - free a platform list
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
void free_platform_list(SW_platform *platform_list);
.in -.6i
.SP
DESCRIPTION
.in +.6i
Free a list of
.I
SW_platform
.R
structures.
.in -.6i
.SP
RETURN VALUES
.in +.6i
none.
.in -.6i
.SP
.ne 5
.H 2 "free_error_info"
.P
NAME
.br
.in +.6i
free_error_info - free an error info structure
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
void free_error_info(SW_error_info *error_info);
.in -.6i
.SP
DESCRIPTION
.in +.6i
Free a
.I
SW_error_info
.R
structure.
.in -.6i
.SP
RETURN VALUES
.in +.6i
none.
.in -.6i
.SP
.ne 5
.H 2 "free_createroot_info"
.P
NAME
.br
.in +.6i
free_createroot_info - free a SW_createroot_info structure.
.in -.6i
.SP
SYNOPSIS
.br
.in +.6i
#include "swmgmt_api.h"
.in -.6i
.SP
.br
.in +.6i
void free_createroot_info(SW_createroot_info *createroot_info);
.in -.6i
.SP
DESCRIPTION
.in +.6i
Free a
.I
SW_createroot_info
.R
structure.
.in -.6i
.SP
RETURN VALUES
.in +.6i
none.
.in -.6i
.SP
.bp
