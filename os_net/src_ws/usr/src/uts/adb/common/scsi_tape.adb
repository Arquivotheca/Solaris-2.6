#pragma	ident	"@(#)scsi_tape.adb	1.11	96/10/17 SMI"

#include <sys/modctl.h>
#include <sys/scsi/scsi.h>
#include <sys/mtio.h>
#include <sys/scsi/targets/stdef.h>
#include <sys/file.h>
#include <sys/stat.h>

scsi_tape
./n"scsi device"16t"req sense p"n{un_sd,X}{un_rqs,X}
+/"mkr_pkt"n{un_mkr_pkt,X}
+/"sbuf_cv"16t"queue_cv"n{un_sbuf_cv,x}16t{un_queue_cv,x}
+/"sbufp"16t"srqbufp"16t"closing cv"n{un_sbufp,X}{un_srqbufp,X}{un_clscv,x}
+/"wait quef"16t"wait quel"16t"runqf"16t"runql"n{un_quef,X}{un_quel,X}{un_runqf,X}{un_runql,X}
+/"mode select p"16t"st_drivetype"16t"dp_size"16t"tmpbuf"n{un_mspl,X}{un_dp,X}{un_dp_size,X}{un_tmpbuf,X}
+/"blkno"16t"oflags"16t"fileno"n{un_blkno,X}{un_oflags,X}{un_fileno,X}
+/"err_fileno"16t"err_blkno"16t"err_resid"16t"fmneeded"n{un_err_fileno,X}{un_err_blkno,X}{un_err_resid,X}{un_fmneeded,x}
+/"dev_t"16t"attached"16t"suspended"n{un_dev,X}{un_attached,B}16t{un_suspended,B}
+/"density_known"16t"curdens"16t"last op"n{un_density_known,B}16t{un_curdens,B}16t{un_lastop,B}
+/"eof"16t"laststate"16t"state"n{un_eof,B}16t{un_laststate,B}16t{un_state,B}
+/"status"16t"retry_ct"16t"tran_rety_ct"16t"read_only"n{un_status,B}16t{un_retry_ct,B}16t{un_tran_retry_ct,B}16t{un_read_only,B}
+/"test_append"16t"arq_enabled"16t"utag_qing"n{un_test_append,B}16t{un_arq_enabled,B}16t{un_untagged_qing,B}
+/"large_xfer?"16t"sbuf_busy"n{un_allow_large_xfer,B}16t{un_sbuf_busy,B}
+/"ncmds"16t"throttle"16t"last_throttle"16t"max_throttle"n{un_ncmds,B}16t{un_throttle,B}16t{un_last_throttle,B}16t{un_max_throttle,B}
+/"persis"16t"persis_flag"16t"flush_on_errors"n{un_persistence,B}16t{un_persist_errors,B}16t{un_flush_on_errors,B}
+/"kbytes_xferred"16t"last_resid"16t"flast_count"n{un_kbytes_xferred,X}{un_last_resid,X}{un_last_count,X}
+/"kstats"16t"rqs_bp"16t"wf"16t"wl"16t"rbl"n{un_stats,X}{un_rqs_bp,X}{un_wf,X}{un_wl,X}{un_rbl,X}
+/"maxdma"16t"bsize"16t"maxbsize"n{un_maxdma,X}{un_bsize,X}{un_maxbsize,X}
+/"minbsize"16t"errno"16t"state_cv"n{un_minbsize,X}{un_errno,X}{un_state_cv,x}
+/"media_state"16t"specified_media_state"n{un_mediastate,X}{un_specified_mediastate,X}
+/"delay_tid"16t"hib_tid"16t"swr_token"n{un_delay_tid,X}{un_hib_tid,X}{un_swr_token,X}
+/"un_comp_page"16t"rsvd status"n{un_comp_page,B}{un_rsvd_status,B}
{END}
