#ifndef lint
#pragma ident "@(#)inst_dsr_media.c 1.19 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_dsr_media.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <malloc.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_sw.h"

/* defines screen presentation order */
#define	_DSR_MEDIA_NUM_OPTIONS	5

#define	_DSR_MEDIA_DISK		0
#define	_DSR_MEDIA_TAPE		1
#define	_DSR_MEDIA_FLOPPY	2
#define	_DSR_MEDIA_NFS		3
#define	_DSR_MEDIA_RSH		4

static int _dsr_media_type_to_index(TDSRALMedia media_type);
static TDSRALMedia _dsr_media_index_to_type(int index);
static int _select_cb(void *cb_data, void *item);
static int _do_dsr_media_path(TList slhandle);
static void dsr_media_device_save(char *media_device);

/* wmenu select/deselect cb client data */
typedef struct {
	char **opts;
	char *add_floppies_note;
	char *add_tapes_note;
	int note_row;
	int note_height;
	int width;
} _pfcDsrMediaCBData;

/*
 * *************************************************************************
 *	Code for the main DSR Media screen
 * *************************************************************************
 */

parAction_t
do_dsr_media(void)
{
	int row;
	u_int fkeys;
	HelpEntry _help;
	char *str;
	DsrSLListExtraData *LLextra;
	int i;
	int ch;
	char *opts[_DSR_MEDIA_NUM_OPTIONS];
	TDSRALMedia curr_media_type;
	parAction_t action;
	int selected;
	int done;
	int really_dirty;
	int width;
	int height;
	_pfcDsrMediaCBData cb_data;
	int num_notes_lines;

	/* flush any premature user input */
	flush_input();

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Select Media for Backup Screen";

	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);
	str = (char *) xmalloc(strlen(MSG_DSR_MEDIA) +
		UI_FS_SIZE_DISPLAY_LENGTH + 1);
	(void) sprintf(str, MSG_DSR_MEDIA,
		UI_FS_SIZE_DISPLAY_LENGTH,
		UI_FS_SIZE_DISPLAY_LENGTH,
		(int) bytes_to_mb(LLextra->archive_size));

	fkeys = F_CONTINUE | F_GOBACK | F_EXIT | F_HELP;

	/* save whatever the current media type is */
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaType, &curr_media_type,
		NULL);

	/*
	 * temporarily set the media type and then get the correct
	 * toggle labels
	 */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	LLextra->history.media_type = DSRALDisk;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &opts[_DSR_MEDIA_DISK],
		NULL);

	LLextra->history.media_type = DSRALTape;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &opts[_DSR_MEDIA_TAPE],
		NULL);

	LLextra->history.media_type = DSRALFloppy;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &opts[_DSR_MEDIA_FLOPPY],
		NULL);

	LLextra->history.media_type = DSRALNFS;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &opts[_DSR_MEDIA_NFS],
		NULL);

	LLextra->history.media_type = DSRALRsh;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &opts[_DSR_MEDIA_RSH],
		NULL);

	/* replace the current media type back to the original value */
	LLextra->history.media_type = curr_media_type;

	selected = _dsr_media_type_to_index(curr_media_type);
	cb_data.opts = opts;

	/* get max length of additional floppies/tapes may be needed note */
	width = COLS - INDENT2 -2;
	height = LINES - HeaderLines - FooterLines;

	cb_data.width = width;
	cb_data.add_floppies_note = (char *) xmalloc(strlen(MSG_DSR_MEDIA) +
		(2 * strlen(LABEL_DSR_MEDIA_MFLOPPY)) + 1);
	(void) sprintf(cb_data.add_floppies_note, MSG_DSR_MEDIA_MULTIPLE,
		LABEL_DSR_MEDIA_MFLOPPY, LABEL_DSR_MEDIA_MFLOPPY);
	cb_data.add_tapes_note = (char *) xmalloc(strlen(MSG_DSR_MEDIA) +
		(2 * strlen(LABEL_DSR_MEDIA_MTAPES)) + 1);
	(void) sprintf(cb_data.add_tapes_note, MSG_DSR_MEDIA_MULTIPLE,
		LABEL_DSR_MEDIA_MTAPES, LABEL_DSR_MEDIA_MTAPES);
	num_notes_lines = MAX(count_lines(cb_data.add_floppies_note, width),
				count_lines(cb_data.add_tapes_note, width));

	/* leave one line at bottom between note and footer */
	num_notes_lines++;

	height -= num_notes_lines + 1;
	cb_data.note_row = LINES - FooterLines - num_notes_lines;
	cb_data.note_height = num_notes_lines;

	/* make sure note is displayed initially, if necessary */
	(void) _select_cb((void *)&cb_data, (void *)selected);

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		/*
		 * If the whole window is likely to be munged and we
		 * want to redraw the whole thing then make sure that we
		 * redraw the header, message, and footer too.
		 */
		if (really_dirty) {
			really_dirty = FALSE;

			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_DSR_MEDIA);

			row = HeaderLines;
			row = wword_wrap(stdscr, row,
				INDENT0, COLS - (2 * INDENT0), str);
			++row;

			wfooter(stdscr, fkeys);
		}

		/* draw and manage the menu input */
		ch = wmenu(stdscr,
			row,
			INDENT1,
			height,
			width,
			show_help, (void *) &_help,
			(Callback_proc *) _select_cb, (void *) &cb_data,
			(Callback_proc *) NULL, (void *) NULL,
			LABEL_DSR_MEDIA_MEDIA,
			cb_data.opts,
			_DSR_MEDIA_NUM_OPTIONS,
			(void *) &selected,
			M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
			fkeys);

		/* handle the final menu selection */
		LLextra->history.media_type =
			_dsr_media_index_to_type(selected);

		/*
		 * help is handled internally to wmenu
		 */
		if (is_goback(ch)) {
			action = parAGoback;
			done = 1;
		} else if (is_exit(ch)) {
			if (confirm_exit(stdscr)) {
				action = parAExit;
				done = TRUE;
			} else {
				really_dirty = TRUE;
			}
		} else if (is_continue(ch)) {
			/*
			 * present screen to get media path now
			 * SUCCESS: they entered a good path and hit OK
			 * FAILURE: they exitted the screen via a
			 * cancel
			 */
			if (_do_dsr_media_path(DsrSLHandle) == SUCCESS) {
				/* media path ok */
				done = 1;
				action = parAContinue;
			} else {
				/* user cancel */
				really_dirty = TRUE;
			}
		}
	}

	/* free stuff */
	for (i = 0; i < _DSR_MEDIA_NUM_OPTIONS; i++) {
		if (opts[i])
			free(opts[i]);
	}

	if (cb_data.add_floppies_note)
		free(cb_data.add_floppies_note);
	if (cb_data.add_tapes_note)
		free(cb_data.add_tapes_note);

	free(str);
	return (action);
}

static int
_dsr_media_type_to_index(TDSRALMedia media_type)
{
	int index;

	switch (media_type) {
	case  DSRALFloppy:
		index = _DSR_MEDIA_FLOPPY;
		break;
	case DSRALTape:
		index = _DSR_MEDIA_TAPE;
		break;
	case DSRALDisk:
		index = _DSR_MEDIA_DISK;
		break;
	case DSRALNFS:
		index = _DSR_MEDIA_NFS;
		break;
	case DSRALRsh:
		index = _DSR_MEDIA_RSH;
		break;
	}

	return (index);
}

static TDSRALMedia
_dsr_media_index_to_type(int index)
{
	TDSRALMedia media_type;

	switch (index) {
	case  _DSR_MEDIA_FLOPPY:
		media_type = DSRALFloppy;
		break;
	case _DSR_MEDIA_TAPE:
		media_type = DSRALTape;
		break;
	case _DSR_MEDIA_DISK:
		media_type = DSRALDisk;
		break;
	case _DSR_MEDIA_NFS:
		media_type = DSRALNFS;
		break;
	case _DSR_MEDIA_RSH:
		media_type = DSRALRsh;
		break;
	}

	return (media_type);
}

static int
_select_cb(void *cb_data, void *item)
{
	int i;

	_pfcDsrMediaCBData *data = (_pfcDsrMediaCBData *) cb_data;
	int index = (int) item;
	TDSRALMedia media_type;
	char *str;

	media_type = _dsr_media_index_to_type(index);
	str = NULL;
	switch (media_type) {
	case  DSRALFloppy:
		str = data->add_floppies_note;
		break;
	case DSRALTape:
		str = data->add_tapes_note;
		break;
	default:
		str = NULL;
	}

	/* clear out old note lines */
	for (i = 0; i < data->note_height; i++) {
		wmove(stdscr, data->note_row + i, 0);
		wclrtoeol(stdscr);
	}

	/* print new additional notes string if we need one */
	if (str) {
		(void) wword_wrap(stdscr,
			data->note_row,
			INDENT0,
			data->width,
			str);
	}

	/* success */
	return (1);
}

/*
 * Get the media path, error check it and store it in the media history.
 */
static int
_do_dsr_media_path(TList slhandle)
{
	HelpEntry _help;
	DsrSLListExtraData *LLextra;
	TDSRALError err;
	char media_device[PATH_MAX + 1];
	char *msg;
	char *egstr;
	char *media_type;
	int row;
	int col;
	u_int fkeys;
	int ret;
	int ch;
	int done;
	int really_dirty;
	char *path_label;
	char *str;
	char buf[PATH_MAX];
	char buf1[PATH_MAX];

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Select Media for Backup Screen";

	(void) LLGetSuppliedListData(slhandle, NULL, (TLLData *)&LLextra);
	if (LLextra->history.media_device)
		(void) strcpy(media_device, LLextra->history.media_device);
	else
		media_device[0] = '\0';

	DsrSLListGetAttr(slhandle,
		DsrSLAttrMediaTypeStr, &media_type,
		NULL);

	DsrSLListGetAttr(slhandle,
		DsrSLAttrMediaTypeEgStr, &egstr,
		NULL);

	msg = (char *) xmalloc(strlen(MSG_CUI_DSR_MEDIA_PATH) +
		strlen(media_type) + 1);
	(void) sprintf(msg, MSG_CUI_DSR_MEDIA_PATH, media_type);
	free(media_type);

	/* path label string */
	path_label = (char *) xmalloc(
		strlen(LABEL_CUI_DSR_MEDIA_PATH) +
		strlen(egstr) + 1);
	(void) sprintf(path_label, LABEL_CUI_DSR_MEDIA_PATH, egstr);
	free(egstr);

	/* seed the screen with the media device if there was a previous one */

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		if (really_dirty) {
			really_dirty = FALSE;
			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_CUI_DSR_MEDIA_PATH);

			row = HeaderLines;
			row = wword_wrap(stdscr, row,
				INDENT0, COLS - (2 * INDENT0), msg);
			row++;

			col = INDENT1;
			mvwprintw(stdscr,
				row,
				col,
				path_label);
			row++;

			fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

			wfooter(stdscr, fkeys);
		}

		ch = wget_field(stdscr,
			LSTRING,
			row,
			col,
			COLS - (2 * col),
			PATH_MAX,
			media_device,
			fkeys);


		if (is_ok(ch) || ch == '\n') {
			/* save the entry to seed it next time */
			dsr_media_device_save(media_device);

			/* did they enter a media device at all? */
			if (!LLextra->history.media_device ||
				!strlen(LLextra->history.media_device)) {
				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_ERROR, APP_ER_DSR_MEDIA_NODEVICE);
				done = FALSE;
				really_dirty = TRUE;
				continue;
			}

			/*
			 * Validate the media type/device entries.
			 *
			 * If it's a tape or floppy tell them to enter
			 * the media now so we can test writing to it to
			 * make sure everything's really ok with the media
			 * first.
			 */
			if (LLextra->history.media_type == DSRALFloppy) {
				(void) sprintf(buf1,
					MSG_DSR_MEDIA_INSERT_FLOPPY_NOTE,
					LABEL_DSR_MEDIA_FLOPPY);
				(void) sprintf(buf, MSG_DSR_MEDIA_INSERT_FIRST,
					LABEL_DSR_MEDIA_FLOPPY,
					buf1);
				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_DSR_MEDIA_INSERT, buf);
				wstatus_msg(stdscr, PLEASE_WAIT_STR);
			} else if (LLextra->history.media_type == DSRALTape) {
				(void) sprintf(buf1,
					MSG_DSR_MEDIA_INSERT_TAPE_NOTE,
					LABEL_DSR_MEDIA_TAPE);
				(void) sprintf(buf, MSG_DSR_MEDIA_INSERT_FIRST,
					LABEL_DSR_MEDIA_TAPE,
					buf1);

				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_DSR_MEDIA_INSERT, buf);
				wstatus_msg(stdscr, PLEASE_WAIT_STR);
			}

			err = DSRALSetMedia(DsrALHandle,
				slhandle,
				LLextra->history.media_type,
				LLextra->history.media_device);
			if (err != DSRALSuccess) {
				if (err == DSRALUnableToWriteMedia &&
					IsIsa("sparc") &&
					LLextra->history.media_type
						== DSRALFloppy) {
					(void) system("eject floppy");
				}
				str = DsrALMediaErrorStr(
					LLextra->history.media_type,
					LLextra->history.media_device,
					err);
				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_ERROR, str);
				if (DsrALMediaErrorIsFatal(err))
					pfcCleanExit(EXIT_INSTALL_FAILURE,
						(void *) 1);
				free(str);
				done = FALSE;
				really_dirty = TRUE;
				continue;
			}

			/*
			 * make sure the media they chose is big enough
			 */
			err = DSRALCheckMediaSpace(slhandle);
			if (err != DSRALSuccess) {
				str = DsrALMediaErrorStr(
					LLextra->history.media_type,
					LLextra->history.media_device,
					err);
				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_ERROR, str);
				if (DsrALMediaErrorIsFatal(err))
					pfcCleanExit(EXIT_INSTALL_FAILURE,
						(void *) 1);
				free(str);
				done = FALSE;
				really_dirty = TRUE;
				continue;
			}

			/*
			 * Ok, if we get here then the media path they
			 * entered is actually ok!
			 */
			done = TRUE;
			ret = SUCCESS;

		} else if (is_cancel(ch)) {
			ret = FAILURE;
			done = TRUE;
		} else if (is_help(ch)) {
			do_help_index(_help.win, _help.type, _help.title);
			really_dirty = TRUE;
		} else
			beep();
	}

	free(msg);
	free(path_label);
	return (ret);
}

static void
dsr_media_device_save(char *media_device)
{
	DsrSLListExtraData *LLextra;

	(void) LLGetSuppliedListData(DsrSLHandle, NULL,
		(TLLData *)&LLextra);

	/* save the text field entry */
	if (LLextra->history.media_device)
		free(LLextra->history.media_device);
	if (!media_device || !strlen(media_device))
		LLextra->history.media_device = NULL;
	else
		LLextra->history.media_device = xstrdup(media_device);
}
