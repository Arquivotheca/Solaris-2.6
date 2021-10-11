/*
 * Copyright (c) 1990-1994, by Sun Microsystems, Inc.
 */
#pragma ident	"@(#)lists.c	1.6	94/03/06 SMI"

/*
 * miscellaneous routines for management of lists of sections,
 * and the functions contained therein.
 */

#include	<stdio.h>

#if defined(__STDC__)
#include	<stdlib.h>
#endif	/* __STDC__ */

#include	"dis.h"
#include	"structs.h"
#define	TOOL	"dis"

extern int	Cflag;
extern int	strncmp(),
		strcmp();
extern char	*strcpy();
extern char	*demangled_name();


/*
 *	build_sections()
 *
 *	create the list of sections to be disassembled
 */

void
build_sections()
{
	extern void	fatal();

	extern	int	nsecs;
	extern	char	**namedsec;
	extern	int	*namedtype;
	extern	char	*fname;
	extern	int	trace;
	extern  int	Fflag;

	extern	SCNLIST		*sclist;
	SCNLIST		*sclisttail;
	SCNLIST		*sectp;

	extern Elf		*elf;
	extern Elf32_Ehdr	*ehdr;
	extern int		archive;
	extern Elf_Arhdr	*mem_header;
	extern int		symtab, debug, line;

	Elf_Scn		*scn;
	Elf32_Shdr	*shdr;
	unsigned 	int	sect = 1;
	int		i;

	/*
	 * read all the section headers in the file.  If the section
	 * is one of the named sections, add it to the list.  If
	 * there were no named sections, and the section is a text
	 * section, add it to the list
	 */

	sclisttail = sclist = NULL;
	/*
	 * section names via -d, -D, and -t options
	 */
	if (nsecs >= 0 && !Fflag) {
		/* Look for the symbol table, debugging and line info */
		sect = 1;
		scn = 0;
		while ((scn = elf_nextscn(elf, scn)) != 0) {
			char *name = 0;
			if ((shdr = elf32_getshdr(scn)) != 0)
				name = elf_strptr(elf, ehdr->e_shstrndx,
				    (long)shdr->sh_name);
			else
				fatal("No section header table");

			if (shdr->sh_type == SHT_SYMTAB)
				symtab = sect;
			else
			if (strcmp(name, ".debug") == 0)
				debug = sect;
			else
			if (strcmp(name, ".line") == 0)
				line = sect;
			sect++;
		}

		for (i = 0; i <= nsecs; i++) {
			sect = 1;
			scn = 0;
			while ((scn = elf_nextscn(elf, scn)) != 0) {
				char *name = 0;
				if ((shdr = elf32_getshdr(scn)) != 0)
					name = elf_strptr(elf, ehdr->e_shstrndx,
					    (long)shdr->sh_name);
				else
					fatal("No section header table");

				if (strcmp(namedsec[i], name) == 0)
					break;
				sect++;
			}

			if (sect > ehdr->e_shnum) {
				if (archive)

					(void) fprintf(stderr,
			"%s: %s[%s]: %s: cannot find section header\n",
			TOOL, fname, mem_header->ar_name, namedsec[i]);

				else
					(void) fprintf(stderr,
				"%s: %s: %s: cannot find section header\n",
					TOOL, fname, namedsec[i]);

				continue;
			}

			if (trace > 0)
				(void) printf("\nsection name is {%s}\n",
				    namedsec[i]);

			if (shdr->sh_type == SHT_NOBITS) {
				if (archive)	/* archive member */

					(void) fprintf(stderr,
		"%s: %s[%s]: %.8s: Can not disassemble a NOBITS section\n",
		TOOL, fname, mem_header->ar_name, namedsec[i]);

				else
					(void) fprintf(stderr,
				"%s: %s: %.8s: Can not dis a NOBITS section\n",
				TOOL, fname, namedsec[i]);

				continue;
			}

			if ((sectp = (SCNLIST *) calloc(1, sizeof (SCNLIST)))
			    == NULL)
				fatal("memory allocation failure");

			sectp->shdr = shdr;
			sectp->scnam = namedsec[i];
			sectp->scnum = sect;
			sectp->stype = namedtype[i];
			if (sclisttail)
				sclisttail->snext = sectp;
			sclisttail = sectp;
			if (sclist == NULL)
				sclist = sectp;
		}
	} else {
		scn = 0;
		while ((scn = elf_nextscn(elf, scn)) != 0) {
			char *name = 0;

			if ((shdr = elf32_getshdr(scn)) == 0)
				fatal("No section header");

			name = elf_strptr(elf, ehdr->e_shstrndx,
			    (long)shdr->sh_name);

			if (shdr->sh_type == SHT_PROGBITS &&
			    shdr->sh_flags == 6) {

				if ((sectp = (SCNLIST *) calloc(1,
				    sizeof (SCNLIST))) == NULL)
					fatal("memory allocation failure");

				sectp->shdr = shdr;
				sectp->scnam = name;
				sectp->scnum = sect;
				sectp->stype = TEXT;
				if (sclisttail)
					sclisttail->snext = sectp;
				sclisttail = sectp;
				if (sclist == NULL)
					sclist = sectp;
			} else
			if (shdr->sh_type == SHT_SYMTAB)
				symtab = sect;
			else
			if (strcmp(name, ".debug") == 0)
				debug = sect;
			else
			if (strcmp(name, ".line") == 0)
				line = sect;
			sect++;
		}
	}
}


/* free the space used by the list of section headers */

void
section_free()
{
	SCNLIST	*sectp;
	SCNLIST	*stemp;
	FUNCLIST	*funcp;
	FUNCLIST	*ftemp;
	extern	SCNLIST	*sclist;

	if (sclist == NULL)
		return;

	sectp = sclist;
	while (sectp) {
		stemp = sectp;
		funcp = sectp->funcs;
		sectp = sectp->snext;
		(void) free(stemp);

		while (funcp) {
			ftemp = funcp;
			funcp = funcp->nextfunc;
			(void) free(ftemp->funcnm);
			(void) free(ftemp);
		}
	}
}



/*  Make a list of all the functions contained in the sections */

void
build_funcs()
{
	extern	Elf	*elf;
	extern  int	symtab;
	extern	void	fatal();
	extern  unsigned int 	strlen();

	extern	SCNLIST	*sclist;

	static  void	add_func(FUNCLIST *, Elf32_Half);

	SCNLIST		*sectp;
	FUNCLIST	*func;
	char		*func_name;

	Elf_Scn		*scn;
	Elf32_Shdr	*shdr;
	Elf_Data	*sym_data;
	int		no_of_symbols, counter;
	Elf32_Sym 	*p;

	if ((scn = elf_getscn(elf, symtab)) == NULL)
		fatal("failed to get the symbol table section");
	if ((shdr = elf32_getshdr(scn)) == NULL)
		fatal("failed to get the section header");
	if (shdr->sh_entsize == 0)
		fatal("the symbol table entry size is 0!");

	sym_data = 0;
	sym_data = elf_getdata(scn, sym_data);
	p = (Elf32_Sym *)sym_data->d_buf;
	no_of_symbols = sym_data->d_size/sizeof (Elf32_Sym);

	p++;	/* the first symbol table entry is skipped */

	for (counter = 1; counter < (no_of_symbols); counter++, p++) {

		for (sectp = sclist; sectp; sectp = sectp->snext)
			if (sectp->scnum == p->st_shndx)
				break;

		if (ELF32_ST_TYPE(p->st_info) == STT_FUNC) {
			if ((func = (FUNCLIST *)
			    calloc(1, sizeof (FUNCLIST))) == NULL)
				fatal("memory allocation failure");

			func_name = (char *)elf_strptr(elf, shdr->sh_link,
			    (size_t)p->st_name);

			if (Cflag)
				func_name = demangled_name(func_name);

			if ((func->funcnm = (char *)
			    calloc(1, (unsigned)(strlen(func_name)+1)))
			    == NULL)
				fatal("memory allocation failure");

			(void) strcpy(func->funcnm, func_name);
			func->faddr = p->st_value;
			func->fcnindex = counter;
			add_func(func, p->st_shndx);
		}
	}
}

/*
 *	add_func()
 *
 *	add func to the list of functions associated with the section
 *	given by sect
 */

static void
add_func(FUNCLIST *func, Elf32_Half sect)
{
	extern	char	*fname;
	extern	SCNLIST	*sclist;

	static short	last_sect = 0;
	static FUNCLIST	*last_func = NULL;
	static char	*last_file = NULL;

	SCNLIST		*sectp;
	FUNCLIST	*funcp;
	FUNCLIST	*backp;
	static int	elist = 1;

	/*
	 * if this function follows the last function added to the list,
	 * the addition can be done quickly
	 */
	if (elist && (last_sect == sect) && last_func &&
	    (last_func->faddr < func->faddr) && last_file &&
	    (strcmp(fname, last_file) == 0)) {
		funcp = last_func->nextfunc;
		func->nextfunc = funcp;
		funcp = func;
		last_func = func;
		elist = 1;
	} else {	/* find the corresponding section pointer */
		for (sectp = sclist; sectp; sectp = sectp->snext) {
			if (sectp->scnum == sect)
				break;
		}

		if (sectp) {
			/* keep the list of functions ordered by address */
			if ((sectp->funcs == NULL) ||
			    (sectp->funcs->faddr > func->faddr)) {
				func->nextfunc = sectp->funcs;
				sectp->funcs = func;
				if (sectp->funcs == NULL) elist = 1;
				else elist = 0;
			} else {
				backp = sectp->funcs;
				funcp = sectp->funcs->nextfunc;
				for (; funcp; funcp = funcp->nextfunc) {
					if (func->faddr <= funcp->faddr) {
						func->nextfunc = funcp;
						backp->nextfunc = func;
						break;
					}
					backp = funcp;
				}
				if (funcp == NULL)
					backp->nextfunc = func;
				elist = 0;
			}

			last_func = func;
			last_sect = sect;
			last_file = fname;
		}

		else
			(void) free(func);
	}
}
