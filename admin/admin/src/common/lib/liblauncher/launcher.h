/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)launcher.h	1.7	94/07/28 SMI"

#define E_ADMIN_OK		0
#define E_ADMIN_BADARG		-101
#define E_ADMIN_EXISTS		-102
#define E_ADMIN_NOCLASS		-103
#define E_ADMIN_TRUNC		-104
#define E_ADMIN_FAIL		-199

typedef void *	admi_handle_t;

typedef enum {
	admin_err_method = -1,
	admin_class_method = 0,
	admin_instance_method
} admin_method_t;

typedef enum {
	admin_err_nameservice = -1,
	admin_ufs_nameservice = 0,
	admin_nis_nameservice,
	admin_nisplus_nameservice,
	admin_dns_nameservice,
	admin_any_nameservice
} admin_nameservice_t;

typedef enum {
	admin_err_transport = -1,
	admin_snag_transport = 0
} admin_transport_t;

typedef enum {
	admin_err_icon = -1,
	admin_xbm_icon = 0,
	admin_cde_icon
} admin_icon_t;

typedef struct {
	int	x_coord;
	int	y_coord;
	int	width;
	int	height;
} admin_geometry_t;


#ifdef __cplusplus
extern "C" {
#endif

/* application registry */

extern int
adma_register(
	const char		*class_name,
	const char		*method_name,
	const char		*executable,
	admin_method_t		method_type,
	admin_nameservice_t	*valid_nameservice,
	int			num_valid_nameservices);

extern int
adma_unregister(
	const char		*the_class,
	const char		*the_method);

extern int
adma_unregister_class(const char *the_class);

extern int
adma_set_executable(
	const char		*the_class,
	const char		*the_method,
	const char		*executable);

extern int
adma_get_executable(
	const char		*the_class,
	const char		*the_method,
	char			*buf,
	size_t			size);

extern int
adma_set_method_type(
	const char		*the_class,
	const char		*the_method,
	admin_method_t		the_type);

extern admin_method_t
adma_get_method_type(
	const char		*the_class,
	const char		*the_method);

extern int
adma_set_valid_nameservices(
	const char		*the_class,
	const char		*the_method,
	admin_nameservice_t	*valid_nameservices,
	int			num_valid_nameservices);

extern admin_nameservice_t *
adma_get_valid_nameservices(
	const char	*the_class,
	const char	*the_method,
	int		*num_valid_nameservices);

extern char **
adma_get_class_methods(
	const char	*the_class,
	int		*num_methods);

extern char **
adma_get_instance_methods(
	const char	*the_class,
	int		*num_methods);

/* class registry */

extern int
admc_register(
	const char	*class_name,
	const char	*display_name,
	const char	*parent_name,
	const char	**children,
	const char	*class_icon,
	admin_icon_t	class_icon_type,
	const char	*instance_icon,
	admin_icon_t	instance_icon_type);

extern int
admc_unregister(const char *class_name);

extern int
admc_delete_registry();

extern int
admc_find_rootclass(char *buf, size_t size);

extern int
admc_set_displayname(const char *key, const char *newdisplay);

extern int
admc_get_displayname(const char *key, char *buf, size_t size);

extern int
admc_reparent(const char *key, const char *newparent);

extern int
admc_get_parentname(const char *key, char *buf, size_t size);

extern char **
admc_get_children(const char *key);

extern int
admc_set_classicon(const char *key, const char *newclassicon);

extern int
admc_get_classicon(const char *key, char *buf, size_t size);

extern int
admc_set_classicontype(const char *key, admin_icon_t newtype);

extern admin_icon_t
admc_get_classicontype(const char *key);

extern int
admc_set_instanceicon(const char *key, const char *newinstanceicon);

extern int
admc_get_instanceicon(const char *key, char *buf, size_t size);

extern int
admc_set_instanceicontype(const char *key, admin_icon_t newtype);

extern admin_icon_t
admc_get_instanceicontype(const char *key);

/* invocation */

extern int
admin_execute_method(
	const char		*a_nameservice,
	const char		*a_transport,
	const char		*a_class,
	const char		*a_method,
	const char		*a_object,
	admin_geometry_t	display_coords,
	...);

extern admi_handle_t
admin_initialize(int *argc, char **argv);

extern admin_nameservice_t
admi_get_nameservice(admi_handle_t handle);

extern admin_transport_t
admi_get_transport(admi_handle_t handle);

extern const char *
admi_get_class(admi_handle_t handle);

extern const char *
admi_get_method(admi_handle_t handle);

extern const char *
admi_get_object(admi_handle_t handle);

extern const char *
admi_get_class_of_object(admi_handle_t handle);

extern const char *
admi_get_display_coords(admi_handle_t handle);

extern void
admi_free_handle(admi_handle_t handle);

/* utility stuff */

extern int
get_value_from_object(
	char		*buf,
	size_t		size,
	const char	*the_class,
	const char	*the_object,
	const char	*the_subclass);

extern int
make_object_from_values(
	char		*buf,
	size_t		size,
	const char	*the_class,
	const char	*the_value,
	const char	**ancestor_classes,
	const char	**ancestor_values,
	int		num_ancestors);

extern int
va_make_object_from_values(
	char		*buf,
	size_t		size,
	const char	*the_class,
	const char	*the_value,
	...);

#ifdef __cplusplus
}
#endif
