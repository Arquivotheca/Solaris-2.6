/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
 * Copyright (c) 1996 Sun Microsystems, Inc.
 * All rights reserved.
 */
#ident	"@(#)elf.c	1.69	96/10/23 SMI" /* from S5R4 1.31 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/thread.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/mman.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/pathname.h>
#include <sys/cmn_err.h>
#include <sys/systm.h>
#include <sys/elf.h>
#include <sys/vmsystm.h>
#include <sys/debug.h>
#ifdef sparc
#include <sys/elf_SPARC.h>
#endif /* sparc */
#ifdef i386
#include <sys/elf_386.h>
#endif /* i386 */
#if defined(__ppc)
#include <sys/elf_ppc.h>
#endif /* ppc */
#include <sys/auxv.h>
#include <sys/exec.h>
#include <sys/prsystm.h>

#include <vm/as.h>
#include <vm/rm.h>
#include <sys/modctl.h>
#include <sys/systeminfo.h>

extern short elfmagic;
extern int at_flags;

static int getelfhead(struct vnode *, Elf32_Ehdr *, caddr_t *, long *);
static int mapelfexec(struct vnode *, Elf32_Ehdr *, caddr_t,
	Elf32_Phdr **, Elf32_Phdr **, Elf32_Phdr **,
	caddr_t *, long *, u_int, long *);

/*ARGSUSED*/
int
elfexec(
	struct vnode *vp,
	struct execa *uap,
	struct uarg *args,
	struct intpdata *idatap,
	int level,
	long *execsz,
	int setid,
	caddr_t exec_file,
	struct cred *cred)
{
	caddr_t		phdrbase = NULL;
	caddr_t 	base = 0;
	int		dlnsize, *aux, resid, error;
	int		fd = -1;
	long		voffset;
	struct vnode	*nvp;
	Elf32_Phdr	*dyphdr = NULL;
	Elf32_Phdr	*stphdr = NULL;
	Elf32_Phdr	*uphdr = NULL;
	Elf32_Phdr	*junk = NULL;
	long		phdrsize;
	int		postfixsize = 0;
	int		i, hsize;
	Elf32_Phdr	*phdrp;
	int		hasu = 0;
	int		hasdy = 0;
	struct proc *p = ttoproc(curthread);
	struct user *up = PTOU(p);
	struct bigwad {
		Elf32_Ehdr	ehdr;
		int		elfargs[NUM_AUX_VECTORS * 2];
		char		dl_name[MAXPATHLEN];
		struct vattr	vattr;
		struct execenv	exenv;
	} *bigwad;	/* kmem_alloc this behemoth so we don't blow stack */
	Elf32_Ehdr	*ehdrp;
	char		*dlnp;

	bigwad = kmem_alloc(sizeof (struct bigwad), KM_SLEEP);
	ehdrp = &bigwad->ehdr;
	dlnp = bigwad->dl_name;

	/*
	 * Obtain ELF and program header information.
	 */
	if ((error = getelfhead(vp, ehdrp, &phdrbase, &phdrsize)) != 0)
		goto out;

	/*
	 * Determine aux size now so that stack can be built
	 * in one shot (except actual copyout of aux image)
	 * and still have this code be machine independent.
	 */
	hsize = ehdrp->e_phentsize;
	phdrp = (Elf32_Phdr *)phdrbase;
	for (i = ehdrp->e_phnum; i > 0; i--) {
		switch (phdrp->p_type) {
		case PT_INTERP:
			hasdy = 1;
			break;
		case PT_PHDR:
			hasu = 1;
		}
		phdrp = (Elf32_Phdr *)((caddr_t)phdrp + hsize);
	}

	/*
	 * If dynamic, aux vector will have at least the 20 words for
	 * the 9 aux types inserted below and 1 for AT_PLATFORM, inserted
	 * by exec_args(). In addition, there are either 8 or 2 words
	 * depending on whether there is a program header or not.
	 * This may be increased by exec_args if there are ISA-specific
	 * types (included in NUM_AUX_VECTORS).
	 */
	if (hasdy)
		args->auxsize = (20 + (hasu ? 8 : 2)) * NBPW;
	else
		args->auxsize = 0;

	aux = bigwad->elfargs;
	/*
	 * Move args to the user's stack.
	 */
	if ((error = exec_args(uap, args, idatap, &aux)) != 0) {
		if (error == -1) {
			error = ENOEXEC;
			goto bad;
		}
		goto out;
	}

	if ((error = mapelfexec(vp, ehdrp, phdrbase, &uphdr, &dyphdr,
	    &stphdr, &base, &voffset, 0, execsz)) != 0)
		goto bad;

#ifdef COFF_SUPPORT	/* XXX - May be required */
	if (stphdr != NULL) {
		/* call coff stuff. */
		if ((error = elf_coffshlib(vp, stphdr, execsz, ehdp)) != 0)
			goto bad;
	}
#endif

	if (uphdr != NULL && dyphdr == NULL)
		goto bad;

	if (dyphdr != NULL) {
		u_int len;
		register Elf32_Phdr *fph = NULL, *lph;

		dlnsize = dyphdr->p_filesz;

		if (dlnsize > MAXPATHLEN || dlnsize <= 0)
			goto bad;

		/*
		 * Read in "interpreter" pathname.
		 */
		if ((error = vn_rdwr(UIO_READ, vp, dlnp, dyphdr->p_filesz,
		    (offset_t)dyphdr->p_offset, UIO_SYSSPACE, 0, (rlim64_t)0,
		    CRED(), &resid)) != 0) {
			uprintf("%s: Cannot obtain interpreter pathname\n",
			    exec_file);
			goto bad;
		}

		if (resid != 0 || dlnp[dlnsize - 1] != '\0')
			goto bad;

		if ((error = lookupname(dlnp, UIO_SYSSPACE, FOLLOW,
		    NULLVPP, &nvp)) != 0) {
			uprintf("%s: Cannot find %s\n", exec_file, dlnp);
			goto bad;
		}

		/*
		 * Setup the "aux" vector.
		 */
		if (uphdr) {
			if (ehdrp->e_type == ET_DYN)
				/* don't use the first page */
				bigwad->exenv.ex_brkbase = (caddr_t)PAGESIZE;
			else
				bigwad->exenv.ex_brkbase = base;
			bigwad->exenv.ex_brksize = 0;
			bigwad->exenv.ex_magic = elfmagic;
			bigwad->exenv.ex_vp = vp;
			setexecenv(&bigwad->exenv);

			*aux++ = AT_PHDR;
			*aux++ = (int)uphdr->p_vaddr + voffset;
			*aux++ = AT_PHENT;
			*aux++ = ehdrp->e_phentsize;
			*aux++ = AT_PHNUM;
			*aux++ = ehdrp->e_phnum;
			*aux++ = AT_ENTRY;
			*aux++ = ehdrp->e_entry + voffset;
		} else {
			if ((error = execopen(&vp, &fd)) != 0) {
				VN_RELE(nvp);
				goto bad;
			}

			*aux++ = AT_EXECFD;
			*aux++ = fd;
		}

		if ((error = execpermissions(nvp, &bigwad->vattr, args)) != 0) {
			VN_RELE(nvp);
			uprintf("%s: Cannot execute %s\n", exec_file, dlnp);
			goto bad;
		}

		/*
		 * Now obtain the ELF header along with the entire program
		 * header contained in "nvp".
		 */
		kmem_free(phdrbase, phdrsize);
		phdrbase = NULL;
		if ((error = getelfhead(nvp, ehdrp, &phdrbase,
		    &phdrsize)) != 0) {
			VN_RELE(nvp);
			uprintf("%s: Cannot read %s\n", exec_file, dlnp);
			goto bad;
		}

		/*
		 * Determine memory size of the "interpreter's" loadable
		 * sections.  This size is then used to obtain the virtual
		 * address of a hole, in the user's address space, large
		 * enough to map the "interpreter".
		 */
		hsize = ehdrp->e_phentsize;
		phdrp = (Elf32_Phdr *)phdrbase;
		for (i = ehdrp->e_phnum; i > 0; i--) {
			if (phdrp->p_type == PT_LOAD) {
				if (fph == NULL)
					fph = phdrp;
				lph = phdrp;
			}
			phdrp = (Elf32_Phdr *)((caddr_t)phdrp + hsize);
		}
		if (fph == NULL) {
			VN_RELE(nvp);
			uprintf("%s: Nothing to load in %s\n", exec_file, dlnp);
			goto bad;
		}
		len = roundup((lph->p_vaddr + lph->p_memsz) -
		    (fph->p_vaddr & PAGEMASK), PAGESIZE);

		error = mapelfexec(nvp, ehdrp, phdrbase, &junk, &junk,
		    &junk, NULL, &voffset, len, execsz);

		VN_RELE(nvp);
		if (error || junk != NULL) {
			uprintf("%s: Cannot map %s\n", exec_file, dlnp);
			goto bad;
		}

		/*
		 * Note: AT_SUN_PLATFORM was filled in via exec_args()
		 */
		*aux++ = AT_BASE;
		*aux++ = voffset;
		*aux++ = AT_FLAGS;
		*aux++ = at_flags;
		*aux++ = AT_PAGESZ;
		*aux++ = PAGESIZE;

		/*
		 * Save uid, ruid, gid and rgid information
		 * for the linker.
		 */
		*aux++ = AT_SUN_UID;
		*aux++ = cred->cr_uid;
		*aux++ = AT_SUN_RUID;
		*aux++ = cred->cr_ruid;
		*aux++ = AT_SUN_GID;
		*aux++ = cred->cr_gid;
		*aux++ = AT_SUN_RGID;
		*aux++ = cred->cr_rgid;
		/*
		 * Hardware capability flag word (performance hints)
		 */
		*aux++ = AT_SUN_HWCAP;
		*aux++ = auxv_hwcap;

		*aux++ = AT_NULL;
		*aux++ = 0;
		postfixsize = (char *)aux - (char *)bigwad->elfargs;
		ASSERT(postfixsize == args->auxsize);
		ASSERT(postfixsize <= NUM_AUX_VECTORS*2*NBPW);
	}

	if (*execsz > btoc(U_CURLIMIT(up, RLIMIT_VMEM))) {
		error = ENOMEM;
		goto bad;
	}

	bzero((caddr_t)up->u_auxv, sizeof (up->u_auxv));
	if (postfixsize) {
		error = execpoststack(args, bigwad->elfargs, postfixsize);
		if (error)
			goto bad;
		ASSERT(postfixsize <= sizeof (up->u_auxv));
		bcopy((caddr_t)bigwad->elfargs, (caddr_t)up->u_auxv,
			postfixsize);
	}

	/*
	 * XXX -- should get rid of this stuff.
	 */
	up->u_exdata.ux_mag = 0413;
	up->u_exdata.ux_entloc = (caddr_t)(ehdrp->e_entry + voffset);

	if (!uphdr) {
		bigwad->exenv.ex_brkbase = base;
		bigwad->exenv.ex_brksize = 0;
		bigwad->exenv.ex_magic = elfmagic;
		bigwad->exenv.ex_vp = vp;
		setexecenv(&bigwad->exenv);
	}
	ASSERT(error == 0);
	goto out;

bad:
	if (fd != -1)		/* did we open the a.out yet */
		(void) execclose(fd);

	psignal(p, SIGKILL);

	if (error == 0)
		error = ENOEXEC;
out:
	if (phdrbase != NULL)
		kmem_free(phdrbase, phdrsize);
	kmem_free(bigwad, sizeof (struct bigwad));
	return (error);
}

/*
 * Read in the ELF header and program header table.
 */
static int
getelfhead(
	struct vnode *vp,
	Elf32_Ehdr *ehdr,
	caddr_t *phdrbase,
	long *phdrsizep)
{
	int resid, error;
	long phdrsize;

	/*
	 * We got here by the first two bytes in ident,
	 * now read the entire ELF header.
	 */
	if ((error = vn_rdwr(UIO_READ, vp, (caddr_t)ehdr,
	    sizeof (Elf32_Ehdr), (offset_t)0, UIO_SYSSPACE, 0,
	    (rlim64_t)0, CRED(), &resid)) != 0)
		return (error);

	if (resid != 0 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
	    (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
	    !elfheadcheck(ehdr->e_ident[EI_DATA], ehdr->e_machine,
		ehdr->e_flags) ||
	    ehdr->e_phentsize == 0 || (ehdr->e_phentsize & 3))
		return (ENOEXEC);

	/*
	 * Determine the size of the program header table
	 * and read it in.
	 */
	*phdrsizep = phdrsize = ehdr->e_phnum * ehdr->e_phentsize;
	*phdrbase = (caddr_t)kmem_alloc(phdrsize, KM_SLEEP);
	return (vn_rdwr(UIO_READ, vp, *phdrbase, phdrsize,
		(offset_t)ehdr->e_phoff, UIO_SYSSPACE, 0, (rlim64_t)0,
		CRED(), &resid));
}

static int
mapelfexec(
	struct vnode *vp,
	Elf32_Ehdr *ehdr,
	caddr_t phdrbase,
	Elf32_Phdr **uphdr,
	Elf32_Phdr **dyphdr,
	Elf32_Phdr **stphdr,
	caddr_t *base,
	long *voffset,
	u_int len,
	long *execsz)
{
	Elf32_Phdr *phdr;
	int i, prot, error;
	caddr_t addr;
	size_t zfodsz;
	int ptload = 0;
	int page;
	off_t offset;
	int hsize = ehdr->e_phentsize;

	if (ehdr->e_type == ET_DYN) {
		/*
		 * Obtain the virtual address of a hole in the
		 * address space to map the "interpreter".
		 */
		map_addr(&addr, len, (offset_t)0, 1);
		if (addr == NULL)
			return (ENOMEM);
		*voffset = (long)addr;
	} else
		*voffset = 0;

	phdr = (Elf32_Phdr *)phdrbase;
	for (i = (int)ehdr->e_phnum; i > 0; i--) {
		switch (phdr->p_type) {
		case PT_LOAD:
			if ((*dyphdr != NULL) && (*uphdr == NULL))
				return (0);

			ptload = 1;
			prot = PROT_USER;
			if (phdr->p_flags & PF_R)
				prot |= PROT_READ;
			if (phdr->p_flags & PF_W)
				prot |= PROT_WRITE;
			if (phdr->p_flags & PF_X)
				prot |= PROT_EXEC;

			addr = (caddr_t)phdr->p_vaddr + *voffset;
			zfodsz = (size_t)phdr->p_memsz - phdr->p_filesz;

			offset = phdr->p_offset;
			if (((long)offset & PAGEOFFSET) ==
			    ((long)addr & PAGEOFFSET) &&
			    (!(vp->v_flag & VNOMAP)))
				page = 1;
			else
				page = 0;

			if (error = execmap(vp, addr, phdr->p_filesz,
			    zfodsz, phdr->p_offset, prot, page))
				goto bad;

			if (base != NULL && addr >= *base)
				*base = addr + phdr->p_memsz;

			*execsz += btoc(phdr->p_memsz);
			break;

		case PT_INTERP:
			if (ptload)
				goto bad;
			*dyphdr = phdr;
			break;

		case PT_SHLIB:
			*stphdr = phdr;
			break;

		case PT_PHDR:
			if (ptload)
				goto bad;
			*uphdr = phdr;
			break;

		case PT_NULL:
		case PT_DYNAMIC:
		case PT_NOTE:
			break;

		default:
			break;
		}
		phdr = (Elf32_Phdr *)((caddr_t)phdr + hsize);
	}
	return (0);
bad:
	if (error == 0)
		error = ENOEXEC;
	return (error);
}

#define	WRT(vp, base, count, offset, rlimit, credp) \
	vn_rdwr(UIO_WRITE, vp, (caddr_t)base, count, (offset_t)offset, \
	UIO_SYSSPACE, 0, (rlim64_t)rlimit, credp, (int *)NULL)

typedef struct {
	Elf32_Nhdr	Nhdr;
	char		name[8];
} Elf32_Note;

extern void setup_old_note_header(Elf32_Phdr *v, proc_t *p);
extern int write_old_elfnotes(proc_t *p, int sig,
	vnode_t *vp, off_t *offsetp, rlim64_t rlimit, cred_t *credp);
extern void setup_note_header(Elf32_Phdr *v, proc_t *p);
extern int write_elfnotes(proc_t *p, int sig,
	vnode_t *vp, off_t *offsetp, rlim64_t rlimit, cred_t *credp);

int
elfnote(
	vnode_t *vp,
	off_t *offsetp,
	int type,
	int descsz,
	caddr_t desc,
	rlim64_t rlimit,
	struct cred *credp)
{
	Elf32_Note note;
	int error;
	offset_t loffset;

	bzero((caddr_t)&note, sizeof (note));
	bcopy("CORE", note.name, 4);
	note.Nhdr.n_type = type;
	note.Nhdr.n_namesz = 5;	/* ABI says this is strlen(note.name)+1 */
	note.Nhdr.n_descsz = roundup(descsz, sizeof (Elf32_Word));
	loffset = (offset_t)*offsetp;
	if (error = WRT(vp, &note, sizeof (note), loffset, rlimit, credp))
		return (error);
	*offsetp += sizeof (note);
	loffset = (offset_t)*offsetp;
	if (error = WRT(vp, desc, note.Nhdr.n_descsz, loffset, rlimit, credp))
		return (error);
	*offsetp += note.Nhdr.n_descsz;
	return (0);
}

int
elfcore(vnode_t *vp, proc_t *p, struct cred *credp, rlim64_t rlimit, int sig)
{
	off_t offset, poffset;
	int error, i, nhdrs;
	int overflow = 0;
	struct seg *seg;
	struct as *as = p->p_as;
	union {
		Elf32_Ehdr ehdr;
		Elf32_Phdr phdr[1];
	} *bigwad;
	u_int bigsize;
	u_int hdrsz;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *v;

	/*
	 * Make sure we have everything we need (registers, etc.).
	 * All other lwps have already stopped and are in an orderly state.
	 */
	ASSERT(p == ttoproc(curthread));
	prstop(0, 0);

	AS_LOCK_ENTER(as, &as->a_lock, RW_WRITER);
	nhdrs = prnsegs(as, 0) + 2;		/* two CORE note sections */
	AS_LOCK_EXIT(as, &as->a_lock);
	hdrsz = nhdrs * sizeof (Elf32_Phdr);

	bigsize = max(sizeof (*bigwad), hdrsz);
	bigwad = kmem_alloc(bigsize, KM_SLEEP);

	ehdr = &bigwad->ehdr;
	bzero((caddr_t)ehdr, sizeof (*ehdr));

	ehdr->e_ident[EI_MAG0] = ELFMAG0;
	ehdr->e_ident[EI_MAG1] = ELFMAG1;
	ehdr->e_ident[EI_MAG2] = ELFMAG2;
	ehdr->e_ident[EI_MAG3] = ELFMAG3;
	ehdr->e_ident[EI_CLASS] = ELFCLASS32;
	ehdr->e_type = ET_CORE;
	elfheadset(&ehdr->e_ident[EI_DATA], &ehdr->e_machine, &ehdr->e_flags);
	ehdr->e_version = EV_CURRENT;
	ehdr->e_phoff = sizeof (Elf32_Ehdr);
	ehdr->e_ehsize = sizeof (Elf32_Ehdr);
	ehdr->e_phentsize = sizeof (Elf32_Phdr);
	ehdr->e_phnum = (unsigned short)nhdrs;
	if (error = WRT(vp, ehdr, sizeof (Elf32_Ehdr), 0LL, rlimit, credp))
		goto done;

	offset = sizeof (Elf32_Ehdr);
	poffset = sizeof (Elf32_Ehdr) + hdrsz;

	v = &bigwad->phdr[0];
	bzero((caddr_t)v, hdrsz);

	setup_old_note_header(&v[0], p);
	v[0].p_offset = poffset;
	poffset += v[0].p_filesz;

	setup_note_header(&v[1], p);
	v[1].p_offset = poffset;
	poffset += v[1].p_filesz;

	AS_LOCK_ENTER(as, &as->a_lock, RW_WRITER);
	i = 2;
	for (seg = AS_SEGP(as, as->a_segs); seg != NULL;
	    seg = AS_SEGP(as, seg->s_next)) {
		caddr_t naddr;
		caddr_t saddr = seg->s_base;
		caddr_t eaddr = seg->s_base + seg->s_size;
		void *tmp = NULL;

		do {
			u_int prot, size;

			prot = pr_getprot(seg, 0, &tmp, &saddr, &naddr);
			if ((size = naddr - saddr) == 0)
				continue;
			if (i == nhdrs) {
				overflow++;
				continue;
			}
			v[i].p_type = PT_LOAD;
			v[i].p_vaddr = (Elf32_Word)saddr;
			v[i].p_memsz = size;
			if (prot & PROT_WRITE)
				v[i].p_flags |= PF_W;
			if (prot & PROT_READ)
				v[i].p_flags |= PF_R;
			if (prot & PROT_EXEC)
				v[i].p_flags |= PF_X;
			if ((prot & PROT_READ) &&
			    (prot & (PROT_WRITE|PROT_EXEC)) != PROT_EXEC &&
			    SEGOP_GETTYPE(seg, saddr) != MAP_SHARED) {
				v[i].p_offset = poffset;
				v[i].p_filesz = size;
				poffset += size;
			}
			i++;
		} while ((saddr = naddr) != eaddr);
		ASSERT(tmp == NULL);
	}
	AS_LOCK_EXIT(as, &as->a_lock);

	if (overflow) {
		cmn_err(CE_WARN, "elfcore: segment overflow");
		error = EIO;
		goto done;
	}
	if (i < nhdrs) {
		cmn_err(CE_WARN, "elfcore: segment underflow");
		error = EIO;
		goto done;
	}

	error = WRT(vp, v, hdrsz, (offset_t)offset, rlimit, credp);
	if (error)
		goto done;
	offset += hdrsz;

	error = write_old_elfnotes(p, sig, vp, &offset, rlimit, credp);
	if (error)
		goto done;

	error = write_elfnotes(p, sig, vp, &offset, rlimit, credp);
	if (error)
		goto done;

	for (i = 2; !error && i < nhdrs; i++) {
		if (v[i].p_filesz == 0)
			continue;
		error = core_seg(p, vp, v[i].p_offset, (caddr_t)v[i].p_vaddr,
		    v[i].p_filesz, rlimit, credp);
	}

done:
	kmem_free(bigwad, bigsize);
	return (error);
}

static struct execsw esw = {
	&elfmagic,
	elfexec,
	elfcore
};

static struct modlexec modlexec = {
	&mod_execops, "exec module for elf", &esw
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlexec, NULL
};

int
_init(void)
{
	return (mod_install(&modlinkage));
}

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}
