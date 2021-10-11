#ifndef lint
#pragma ident "@(#)pfgprocess.h 1.14 96/08/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgprocess.h
 * Group:	ttinstall
 * Description:
 */

#ifndef	_PFGPROCESS_H
#define	_PFGPROCESS_H

extern parAction_t pfgProcessAllocateSvcQuery(void);
extern parAction_t pfgProcessAutoQuery(void);
extern parAction_t pfgProcessClientParams(void);
extern parAction_t pfgProcessClients(void);
extern parAction_t pfgProcessDsrALGenerateProgress(void);
extern parAction_t pfgProcessDsrAnalyze(void);
extern parAction_t pfgProcessDsrFSRedist(void);
extern parAction_t pfgProcessDsrFSSummary(void);
extern parAction_t pfgProcessDsrMedia(void);
extern parAction_t pfgProcessDsrSpaceReq(void);
extern parAction_t pfgProcessFilesys(void);
extern parAction_t pfgProcessInit(void);
extern parAction_t pfgProcessIntro(parWin_t);
extern parAction_t pfgProcessLocales(void);
extern parAction_t pfgProcessOs(void);
extern parAction_t pfgProcessPreQuery(void);
extern parAction_t pfgProcessProgress(void);
extern parAction_t pfgProcessReboot(void);
extern parAction_t pfgProcessRemquery(void);
extern parAction_t pfgProcessSummary(void);
extern parAction_t pfgProcessSw(void);
extern parAction_t pfgProcessSwQuery(void);
extern parAction_t pfgProcessUpgrade(void);
extern parAction_t pfgProcessUpgradeProgress(void);
extern parAction_t pfgProcessUseDisks(void);

extern void pfgSetAction(parAction_t action);
extern void pfgUnmountOrSwapError(void);

#endif	/* _PFGPROCESS_H */
