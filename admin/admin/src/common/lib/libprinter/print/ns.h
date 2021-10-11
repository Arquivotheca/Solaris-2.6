/*
 * Copyright (c) 1994 by SunSoft, Inc.
 */
#ifndef _NS_H
#define _NS_H

#pragma ident  "@(#)ns.h 1.4     95/06/05 SunSoft"

#ifdef _cplusplus
extern "C" {
#endif

/*
 *		Name Service Common Keys/types for lookup
 */
#define NS_KEY_BSDADDR			"bsdaddr"
#define NS_KEY_USE		 	"use"
#define NS_KEY_ALL		 	"all"
#define NS_KEY_GROUP		 	"group"
#define NS_KEY_LIST		 	"list"

#define NS_KEY_PRINTER_TYPE             "printer-type"
#define NS_KEY_DESCRIPTION              "description"

/*
 *		Name Service reserved names for lookup
 */
#define NS_NAME_DEFAULT		"_default"
#define NS_NAME_ALL		"_all"

/*
 *		Name Services supported
 */
#define NS_SVC_USER		"user"
#define NS_SVC_PRINTCAP		"printcap"
#define NS_SVC_ETC		"etc"
#define NS_SVC_NIS		"nis"
#define NS_SVC_NISPLUS		"nisplus"
#define NS_SVC_XFN		"xfn"

/*
 *		Known Protocol Extensions
 */
#define NS_EXT_SOLARIS		"solaris"
#define NS_EXT_GENERIC		"extensions" /* same as SOLARIS */
#define NS_EXT_HPUX		"hpux"
#define NS_EXT_DEC		"dec"

/*  BSD binding address structure */
struct ns_bsd_addr {
  char	*server;        /* server name */
  char	*printer;       /* printer name or NULL */
  char	*extension;     /* RFC-1179 conformance */
};
typedef struct ns_bsd_addr ns_bsd_addr_t;

/* Key/Value pair structure */
struct ns_kvp {
  char *key;              /* key */
  void *value;            /* value converted */
};
typedef struct ns_kvp ns_kvp_t;

/* Printer Object structure */
struct ns_printer {
  char     *name;         /* primary name of printer */
  char     **aliases;     /* aliases for printer */
  char     *source;       /* name service derived from */
  ns_kvp_t **attributes;  /* key/value pairs. */
};
typedef struct ns_printer ns_printer_t ;

/* functions to get/put printer objects */
extern ns_printer_t *ns_printer_create(char *, char **, char *, ns_kvp_t **);
extern ns_printer_t *ns_printer_get_name(const char *, const char *);
extern ns_printer_t **ns_printer_get_list(const char *);
extern int          ns_printer_put(const ns_printer_t *);
extern void         ns_printer_destroy(ns_printer_t *);

/* functions to manipulate key/value pairs */
extern void         *ns_get_value(const char *, const ns_printer_t *);
extern char         *ns_get_value_string(const char *, const ns_printer_t *);
extern int          ns_set_value(const char *, const void *, ns_printer_t *);
extern int          ns_set_value_from_string(const char *, const char *,
						ns_printer_t *);

extern ns_printer_t	*dyn_ns_printer_get_name(
			    const char	*name,
			    const char	*context);
extern ns_printer_t	**dyn_ns_printer_get_list(const char *context);
extern int		dyn_ns_printer_put(ns_printer_t *printer);
extern void		dyn_ns_printer_destroy(ns_printer_t *printer);

extern int	dyn_ns_set_value(
		    const char		*key,
		    const void		*value,
		    ns_printer_t	*printer);
extern int	dyn_ns_set_value_from_string(
		    const char		*key,
		    const char		*value,
		    ns_printer_t	*printer);
extern void	*dyn_ns_get_value(
		    const char		*key,
		    ns_printer_t	*printer);
extern char	*dyn_ns_get_value_string(
		    const char		*key,
		    ns_printer_t	*printer);

#ifdef _cplusplus
}
#endif

#endif /* _NS_H */
