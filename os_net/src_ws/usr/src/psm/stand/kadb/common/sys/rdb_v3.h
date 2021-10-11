/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _RDB_V3_H_RPCGEN
#define	_RDB_V3_H_RPCGEN

#include "rpc/rpc.h"
#define	INVALID_PID	((ProcessId) -1)

typedef u_long Phandle;

typedef u_long ProcessId;

typedef u_long Reg;

typedef u_int RegNumber;

enum Status {
	RDB_OK = 0,
	RDB_ERROR = -1
};
typedef enum Status Status;

enum Error {
	RDBERR_INVAL = 1,
	RDBERR_ADDR = 2,
	RDBERR_PERM = 3,
	RDBERR_SYSERR = 30
};
typedef enum Error Error;

struct rdb_error {
	Error err;
	union {
		char *rsyserr;
	} rdb_error_u;
};
typedef struct rdb_error rdb_error;

struct status_reply {
	Status stat;
	union {
		rdb_error error;
	} status_reply_u;
};
typedef struct status_reply status_reply;

enum ProcessState {
	PS_NONE = 0,
	PS_CREATED = 1,
	PS_INIT = 2,
	PS_STOP = 3,
	PS_RUN = 4,
	PS_ERROR = 5,
	PS_EXIT = 6,
	PS_ABORT = 7
};
typedef enum ProcessState ProcessState;

enum MachType {
	RDB_MSPARC = 0,
	RDB_MSPARC64 = 1
};
typedef enum MachType MachType;

enum StopReason {
	STOPREASON_BKPT = 0,
	STOPREASON_INTR = 1,
	STOPREASON_TRAP = 2
};
typedef enum StopReason StopReason;

enum XdrType {
	XDR_BYTE = 0,
	XDR_HALF = 1,
	XDR_WORD = 2
};
typedef enum XdrType XdrType;

enum AddrType {
	ADDR_V32 = 0,
	ADDR_A32 = 1,
	ADDR_P = 2
};
typedef enum AddrType AddrType;

struct alt_addr32 {
	u_long asi;
	u_long addr;
};
typedef struct alt_addr32 alt_addr32;

struct phys_addr {
	u_long addr_hi;
	u_long addr_lo;
};
typedef struct phys_addr phys_addr;

struct mem_addr {
	AddrType type;
	union {
		u_long v32_addr;
		alt_addr32 a32_addr;
		phys_addr p_addr;
	} mem_addr_u;
};
typedef struct mem_addr mem_addr;

struct Datadefn {
	enum XdrType datatype;
	u_long count;
};
typedef struct Datadefn Datadefn;

struct read_req {
	Phandle ph;
	mem_addr startaddr;
	Datadefn datainfo;
};
typedef struct read_req read_req;

typedef short hword;

struct mem_data {
	XdrType datatype;
	union {
		struct {
			u_int bdata_len;
			char *bdata_val;
		} bdata;
		struct {
			u_int ldata_len;
			long *ldata_val;
		} ldata;
		struct {
			u_int hdata_len;
			hword *hdata_val;
		} hdata;
	} mem_data_u;
};
typedef struct mem_data mem_data;

struct read_reply {
	Status stat;
	union {
		rdb_error error;
		mem_data data;
	} read_reply_u;
};
typedef struct read_reply read_reply;

struct write_req {
	Phandle ph;
	mem_addr startaddr;
	mem_data data;
};
typedef struct write_req write_req;

enum SparcRegType {
	SR_IREG = 0,
	SR_ISTATE = 1,
	SR_FREG = 2,
	SR_FSTATE = 3,
	SR_FQ = 4,
	SR_CREG = 5,
	SR_CSTATE = 6,
	SR_CQ = 7
};
typedef enum SparcRegType SparcRegType;

struct sparc_reg_no {
	SparcRegType type;
	u_long regno;
};
typedef struct sparc_reg_no sparc_reg_no;

struct regnum {
	MachType mach;
	union {
		sparc_reg_no regnum;
	} regnum_u;
};
typedef struct regnum regnum;

struct getregs_req {
	Phandle ph;
	regnum start_reg;
	long count;
};
typedef struct getregs_req getregs_req;

struct getregs_reply {
	Status stat;
	union {
		rdb_error error;
		struct {
			u_int regdata_len;
			Reg *regdata_val;
		} regdata;
	} getregs_reply_u;
};
typedef struct getregs_reply getregs_reply;

struct setregs_req {
	Phandle ph;
	regnum start_reg;
	struct {
		u_int regdata_len;
		Reg *regdata_val;
	} regdata;
};
typedef struct setregs_req setregs_req;
/*
 * Definitions which map all SPARC registers into a
 * flat numbering scheme.
 */

/*
 * #define'd names which begin with '_' are not intended to be referenced
 * outside of this header file!
 */

/* ------------------------------------------------------------------------- */

	/*
	 * Register numbers (values to put in a RegNumber).
	 * Note that up to 16 8-byte entries are allowed,
	 * for an FP Queue or a CP Queue.
	 */
/* Register numbers for general-purpose IU registers... */
#define	RN_First_IR	(0)
#define	RN_IR(n)	((RegNumber)(n))
#define	RN_GBL(n)	RN_IR(0+(n))	/* the Global	registers */
#define	RN_OUT(n)	RN_IR(8+(n))	/* the Out	registers */
#define	RN_LCL(n)	RN_IR(16+(n))	/* the Local	registers */
#define	RN_IN(n)	RN_IR(24+(n))	/* the In	registers */
#define	RN_Last_IR	(32)
	/*
	 * The names of some well-known registers, for convenience.
	 */
#define	RN_SP		RN_IR(14)	/* the Stack Pointer */
#define	RN_FP		RN_IR(30)	/* the Frame Pointer */


/* Register numbers for IU state registers... */
#define	RN_First_IS	((RegNumber)(0))
#define	RN_WIM		((RegNumber)(0))
#define	RN_TBR		((RegNumber)(1))
#define	RN_PSR		((RegNumber)(2))
#define	RN_PC		((RegNumber)(3))
#define	RN_NPC		((RegNumber)(4))
#define	RN_Y		((RegNumber)(5))
#define	RN_ASR0		RN_Y
#define	RN_ASR(n)	((RegNumber)(RN_ASR0 + (n)))
#define	RN_Last_IS	RN_ASR(32)


/* Register numbers for FP registers... */
#define	RN_First_FR	((RegNumber)(0))
#define	RN_FR(n)	((RegNumber((n))
#define	RN_Last_FR	RN_FR(32)


/* Register numbers for FP state registers... */
#define	RN_First_FS	((RegNumber)(0))
#define	RN_FSR		((RegNumber)(0))
#define	RN_FPQC		((RegNumber)(1)) /* Pseudo-register */
#define	RN_FPEN		((RegNumber)(2)) /* Pseudo-register */
#define	RN_Last_FS	RN_FPEN


/* Register numbers for FQ entries... */
#define	RN_FQ(n)	((RegNumber)(n))


/* Register numbers for CP registers... */
#define	RN_First_CR	((RegNumber)(0))
#define	RN_CR(n)	((RegNumber((n))
#define	RN_Last_CR	RN_CR(32)


/* Register numbers for CP state registers... */
#define	RN_First_CS	((RegNumber)(0))
#define	RN_CSR		((RegNumber)(0))
#define	RN_CPQC		((RegNumber)(1)) /* Pseudo-register */
#define	RN_CPEN		((RegNumber)(2)) /* Pseudo-register */
#define	RN_Last_CS	RN_CPEN


/* Register numbers for CQ entries... */
#define	RN_CQ(n)	((RegNumber)(n))



struct cont_req {
	Phandle ph;
	u_long callback_prog;
	mem_addr addr;
};
typedef struct cont_req cont_req;

typedef char *str_el;

typedef struct {
	u_int str_arr_len;
	str_el *str_arr_val;
} str_arr;

enum download_style {
	MAY_USE_DOWNLOAD = 0,
	MUST_USE_DOWNLOAD = 1,
	BOOT = 2
};
typedef enum download_style download_style;

enum force_load {
	MAY_RELOAD = 0,
	MUST_RELOAD = 1,
	DONT_RELOAD = 2
};
typedef enum force_load force_load;

struct args {
	str_arr argv;
	str_arr envp;
};
typedef struct args args;

struct download_cmd {
	download_style style;
	union {
		args args1;
		args args2;
		char *boot_cmd;
	} download_cmd_u;
};
typedef struct download_cmd download_cmd;

struct init_req {
	Phandle ph;
	force_load do_load;
	u_long callback_prog;
	download_cmd cmd;
};
typedef struct init_req init_req;

struct init_reply {
	Phandle ph;
	Status request_status;
	bool_t will_download;
};
typedef struct init_reply init_reply;

struct connect_info {
	long flags[2];
	MachType mach;
};
typedef struct connect_info connect_info;

struct connect_reply {
	Status stat;
	union {
		rdb_error error;
		connect_info info;
	} connect_reply_u;
};
typedef struct connect_reply connect_reply;

struct proc_ent {
	ProcessId pid;
	char *idstring;
	char *entry;
};
typedef struct proc_ent proc_ent;

struct proc_list_reply {
	Status stat;
	union {
		rdb_error error;
		struct {
			u_int proc_list_len;
			proc_ent *proc_list_val;
		} proc_list;
	} proc_list_reply_u;
};
typedef struct proc_list_reply proc_list_reply;
/* (flag word bits follow) */
#define	FPU_PRESENT_BIT	 (0x0001)	/* word 0, bit 0 */
#define	CP_PRESENT_BIT	 (0x0002)	/* word 0, bit 1 */
#define	CAN_DO_BKPTS_BIT (0x0004)	/* word 0, bit 2 */
#define	CAN_SGL_STEP_BIT (0x0008)	/* word 0, bit 3 */

#define	FPU_IS_PRESENT(flags) ((flags[0] & FPU_PRESENT_BIT) != 0)
#define	CP_IS_PRESENT(flags) ((flags[0] & CP_PRESENT_BIT) != 0)
#define	CAN_DO_BREAKPOINTS(flags) ((flags[0] & CAN_DO_BKPTS_BIT) != 0)
#define	CAN_SINGLE_STEP(flags) ((flags[0] & CAN_SGL_STEP_BIT) != 0)


struct ProcStatus {
	ProcessState pstate;
	union {
		StopReason reason;
		int exitcode;
		int traptype;
		int aborttype;
	} ProcStatus_u;
};
typedef struct ProcStatus ProcStatus;

struct proc_stat_msg {
	ProcessId pid;
	Phandle ph;
	ProcStatus pstat;
};
typedef struct proc_stat_msg proc_stat_msg;

struct proc_status_reply {
	Status stat;
	union {
		rdb_error error;
		proc_stat_msg proc_stat;
	} proc_status_reply_u;
};
typedef struct proc_status_reply proc_status_reply;

struct Breakpoint {
	Phandle ph;
	mem_addr addr;
};
typedef struct Breakpoint Breakpoint;

enum detach_state {
	CONTINUE = 0,
	STOPPED = 1,
	DESTROY = 2
};
typedef enum detach_state detach_state;

struct detach_req {
	Phandle ph;
	detach_state state;
};
typedef struct detach_req detach_req;

enum dl_req {
	DLREQ_SEND = 0,
	DLREQ_RESEND = 1,
	DLREQ_ABORT = 2
};
typedef enum dl_req dl_req;

enum dr_stat {
	DRSTAT_SEC = 0,
	DRSTAT_DONE = 1,
	DRSTAT_ERROR = 2,
	DRSTAT_ABORTACK = 3
};
typedef enum dr_stat dr_stat;

struct download_data {
	mem_addr addr;
	mem_data data;
};
typedef struct download_data download_data;

struct download_reply {
	dr_stat stat;
	union {
		download_data data;
		mem_addr dr_entry;
	} download_reply_u;
};
typedef struct download_reply download_reply;

struct download_done {
	Status download_status;
	ProcessState proc_state;
};
typedef struct download_done download_done;

#define	RDBPROG ((u_long)0x24700000)
extern struct rpcgen_table rdbprog_3_table[];
extern rdbprog_3_nproc;
#define	RDBVERS_3 ((u_long)3)
#define	RDB_CONNECT ((u_long)1)
extern  connect_reply * rdb_connect_3();
#define	RDB_CREATE_PROC ((u_long)2)
extern  proc_status_reply * rdb_create_proc_3();
#define	RDB_INIT_PROC ((u_long)3)
extern  init_reply * rdb_init_proc_3();
#define	RDB_ATTACH_PROC ((u_long)4)
extern  proc_status_reply * rdb_attach_proc_3();
#define	RDB_DETACH_PROC ((u_long)5)
extern  proc_status_reply * rdb_detach_proc_3();
#define	RDB_DESTROY_PROC ((u_long)6)
extern  status_reply * rdb_destroy_proc_3();
#define	RDB_DISCONNECT ((u_long)7)
extern  status_reply * rdb_disconnect_3();
#define	RDB_CONTINUE ((u_long)8)
extern  status_reply * rdb_continue_3();
#define	RDB_SINGLE_STEP ((u_long)9)
extern  status_reply * rdb_single_step_3();
#define	RDB_STOP_PROC ((u_long)10)
extern  status_reply * rdb_stop_proc_3();
#define	RDB_READ_MEM ((u_long)11)
extern  read_reply * rdb_read_mem_3();
#define	RDB_WRITE_MEM ((u_long)12)
extern  status_reply * rdb_write_mem_3();
#define	RDB_GET_REGS ((u_long)13)
extern  getregs_reply * rdb_get_regs_3();
#define	RDB_SET_REGS ((u_long)14)
extern  status_reply * rdb_set_regs_3();
#define	RDB_SET_BRKPT ((u_long)15)
extern  status_reply * rdb_set_brkpt_3();
#define	RDB_CLR_BRKPT ((u_long)16)
extern  status_reply * rdb_clr_brkpt_3();
#define	RDB_GET_STATE ((u_long)17)
extern  proc_status_reply * rdb_get_state_3();
#define	RDB_GET_PROCLIST ((u_long)18)
extern  proc_list_reply * rdb_get_proclist_3();

#define	RDB_CALLBACKPROG ((u_long)-1)
extern struct rpcgen_table rdb_callbackprog_3_table[];
extern rdb_callbackprog_3_nproc;
#define	RDB_CALLBACKVERS_3 ((u_long)3)
#define	RDB_PROC_STOPPED ((u_long)1)
extern  void * rdb_proc_stopped_3();
#define	RDB_DOWNLOAD ((u_long)2)
extern  download_reply * rdb_download_3();
#define	RDB_DOWNLOAD_DONE ((u_long)3)
extern  void * rdb_download_done_3();

/* the xdr functions */
bool_t xdr_Phandle();
bool_t xdr_ProcessId();
bool_t xdr_Reg();
bool_t xdr_RegNumber();
bool_t xdr_Status();
bool_t xdr_Error();
bool_t xdr_rdb_error();
bool_t xdr_status_reply();
bool_t xdr_ProcessState();
bool_t xdr_MachType();
bool_t xdr_StopReason();
bool_t xdr_XdrType();
bool_t xdr_AddrType();
bool_t xdr_alt_addr32();
bool_t xdr_phys_addr();
bool_t xdr_mem_addr();
bool_t xdr_Datadefn();
bool_t xdr_read_req();
bool_t xdr_hword();
bool_t xdr_mem_data();
bool_t xdr_read_reply();
bool_t xdr_write_req();
bool_t xdr_SparcRegType();
bool_t xdr_sparc_reg_no();
bool_t xdr_regnum();
bool_t xdr_getregs_req();
bool_t xdr_getregs_reply();
bool_t xdr_setregs_req();
bool_t xdr_cont_req();
bool_t xdr_str_el();
bool_t xdr_str_arr();
bool_t xdr_download_style();
bool_t xdr_force_load();
bool_t xdr_args();
bool_t xdr_download_cmd();
bool_t xdr_init_req();
bool_t xdr_init_reply();
bool_t xdr_connect_info();
bool_t xdr_connect_reply();
bool_t xdr_proc_ent();
bool_t xdr_proc_list_reply();
bool_t xdr_ProcStatus();
bool_t xdr_proc_stat_msg();
bool_t xdr_proc_status_reply();
bool_t xdr_Breakpoint();
bool_t xdr_detach_state();
bool_t xdr_detach_req();
bool_t xdr_dl_req();
bool_t xdr_dr_stat();
bool_t xdr_download_data();
bool_t xdr_download_reply();
bool_t xdr_download_done();
struct rpcgen_table {
	char	*(*proc)();
	xdrproc_t	xdr_arg;
	unsigned	len_arg;
	xdrproc_t	xdr_res;
	unsigned	len_res;
};

#endif /* !_RDB_V3_H_RPCGEN */