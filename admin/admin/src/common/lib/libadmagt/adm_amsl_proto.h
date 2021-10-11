/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef	_adm_amsl_proto_h
#define	_adm_amsl_proto_h

#pragma	ident	"@(#)adm_amsl_proto.h	1.17	95/06/02 SMI"

/*
 * FILE:  adm_amsl_proto.h
 *
 *	Admin Framework class agent header file exporting AMSL function
 *	prototype definitions.
 */

/* Admin class agent external function prototype definitions */

#ifdef __cplusplus
extern "C" {
#endif

extern void   amsl_dispatch(u_int, char *, char *, char *, u_int,
		    struct timeval, u_int);
extern int    amsl_err(struct amsl_req *, int, ...);
extern int    amsl_err1(struct amsl_req *, int, char *);
extern int    amsl_err2(char *, int, ...);
extern int    amsl_err3(int, char **);
extern void   amsl_err_netmgt(int *, char **);
extern void   amsl_err_dbg(char *, ...);
extern void   amsl_err_syslog(int, ...);
extern void   amsl_err_syslog1(char *);
extern int    amsl_invoke(struct amsl_req *);
extern int    amsl_local_dispatch(u_int, Adm_requestID *, char *, char *,
		    char *, Adm_data *, Adm_data *, Adm_error *, char *,
		    char *);
extern int    amsl_local_verify(struct amsl_req *);
extern int    amsl_log(struct amsl_req *, u_int, char *, ...);
extern int    amsl_log1(char *, char *);
extern int    amsl_log2(struct amsl_req *, u_int, u_int, char *, char *,
		    char *);
extern int    amsl_log3(u_int, ...);
extern boolean_t amsl_verify(u_int, char *, char *, char *, u_int,
		    struct timeval, u_int);
extern int    build_argv(struct amsl_req *, struct Adm_data *, char ***);
extern int    build_env(struct amsl_req *, struct Adm_data *, int, char ***);
extern struct amsl_req *find_my_req();
extern struct amsl_req *find_req(u_int);
extern int    check_auth(struct amsl_req *);
extern int    check_pipes(struct pollfd []);
extern void   close_pipe(struct pollfd *);
extern void   free_amsl(struct amsl_ctl **);
extern void   free_buffs(struct amsl_req *);
extern void   free_pipes(struct pollfd []);
extern void   free_req(struct amsl_req *);
extern int    get_auth(struct amsl_req *);
extern int    get_input_parms(struct amsl_req *, struct Adm_data *);
extern int    get_system_parms(struct amsl_req *);
extern int    grow_buff(struct bufctl *);
extern int    init_amsl(time_t, u_int, char *, char *, char *, u_int);
extern int    init_buffs(struct amsl_req *, struct pollfd []);
extern int    init_pipes(struct amsl_req *, struct pollfd []);
extern int    init_req(struct amsl_req **, Adm_requestID *, Adm_error *);
extern int    link_req(struct amsl_req *);
extern int    put_action_header(struct amsl_req *);
extern int    put_callback(struct amsl_req *, int);
extern int    put_output_parms(struct amsl_req *, struct Adm_data *);
extern int    put_version_header(struct amsl_req *);
extern int    put_weakauth_header(struct amsl_req *, char *);
extern int    set_local_auth(struct amsl_req *);
extern int    set_pipes(struct amsl_req *, struct pollfd [], u_int);
extern int    unlink_req(struct amsl_req *);

/* SNM Agent Services library non-public function prototype definitions */
extern u_int  _netmgt_get_auth_flavor(uid_t *, gid_t *, u_int *, gid_t [],
		    u_int, char[]);
extern u_int  _netmgt_get_request_sequence();
extern u_int  _netmgt_get_child_sequence();
extern pid_t  _netmgt_get_child_pid(u_int);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_amsl_proto_h */
