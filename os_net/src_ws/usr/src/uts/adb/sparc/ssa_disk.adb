#pragma ident   "@(#)ssa_disk.adb 1.7     96/09/14 SMI"

#include <sys/scsi/scsi.h>
#include <sys/dkio.h>
#include <sys/scsi/targets/ssddef.h>

ssa_disk
./n"devp"16t"rqs pkt"16t"rqs bp"n{un_sd,X}{un_rqs,X}{un_rqs_bp,X}
+/"rqs sema:"n
.$<<sema{OFFSETOK}
+/n"sbufp"16t"srqbufp"16t"sbuf cv"n{un_sbufp,X}{un_srqbufp,X}{un_sbuf_cv,x}
+/n"ocmap"n{un_ocmap,36B}
+/n"map"n{un_map,16X}
+/n"offset"n{un_offset,8X}
+/"geom:"n
.$<<dk_geom{OFFSETOK}
+/n"last pkt reason"n{un_last_pkt_reason,B}
+/"vtoc:"n
.$<<dk_vtoc{OFFSETOK}
+/n"diskhd"n{un_utab,6X}
+/n"stats"n{un_stats,X}
+/n"part_stats"n{un_pstats,8X}
+/n"oclose sema:"n
.$<<sema{OFFSETOK}
+/n"err blkno"16t"capacity"n{un_err_blkno,X}{un_capacity,X}
+/n"exclopen"8t"gvalid"8t"state"8t"last state"n{un_exclopen,B}16t{un_gvalid,B}{un_state,B}{un_last_state,B}
+/n"format progress"n{un_format_in_progress,B}
+/n"start stop issued"n{un_start_stop_issued,B}
+/n"asciilabel"n{un_asciilabel,128c}
+/n"throttle"8t"save throttle"8t"ncmds"8t"tagflags"n{un_throttle,x}8t{un_save_throttle,x}8t{un_ncmds,x}{un_tagflags,X}
+/n"sbuf busy"8t"resvd status"n{un_sbuf_busy,x}16t{un_resvd_status,x}
+/n"mhd token"8t"resvd timeid"8t"reset throttle_timeid"n{un_mhd_token,X}{un_resvd_timeid,X}{un_reset_throttle_timeid,X}
+/n"restart timeid"16t"sdrestart timeid"n{un_restart_timeid,X}{un_reissued_timeid,X}
+/n"device id"16t"max transfer size"n{un_devid,X}{un_max_xfer_size,X}{END}
