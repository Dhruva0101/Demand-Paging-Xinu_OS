/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

LOCAL	newpid();

extern pt_t *global_pt[]; // array of 4 pointers declared in already (paginginit.c) so extern
extern bs_map_t bsm_tab[]; // backing store declared in already (bsm.c) so extern
extern fr_map_t frm_tab[]; // frame table declared in already (frame.c) so extern

/*------------------------------------------------------------------------
 * vcreate - create a process similar to the create syscall, but with a 
 * virtual heap rather than using the shared physical heap.
 *------------------------------------------------------------------------
 */

SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	int frm, pid, i, store_index;
	unsigned long pd_phys_addr;
	pd_t *pd;

	struct mblock *vhead; // head node of vmemlist

	// Validate input
	if (hsize <=0 || priority <=0){
		return SYSERR;
	}

	// Create base process using existing create()
	pid = create(procaddr, ssize, priority, name, nargs, args);
	if(pid == SYSERR){
		return SYSERR;
	}

	// Allocate a frame for this process's PD
	if(get_frm(&frm) == SYSERR){
		return SYSERR;
	}

	frm_tab[frm].fr_status = FRM_MAPPED;
    frm_tab[frm].fr_pid    = pid;
    frm_tab[frm].fr_vpno   = -1;
    frm_tab[frm].fr_refcnt = 1;
    frm_tab[frm].fr_type   = FR_DIR;
    frm_tab[frm].fr_dirty  = 0;
	
	pd_phys_addr = (FRAME0 + frm) * NBPG;
	pd = (pd_t *)pd_phys_addr;

	disable(ps);

	for(i =0; i < 1024; i++){
		pd[i].pd_pres     =(i < 4);  // page table is present
        pd[i].pd_write    =1;  // writable = 1, read-only = 0
        pd[i].pd_user     =0; 
		pd[i].pd_base     =(i < 4) ? ((unsigned long)global_pt[i]) >> 12:0 ;
	}

	//save pd base(phys) in proctab for context switching
	proctab[pid].pdbr = pd_phys_addr;

	// allocate a private bs for heap
	if(get_bsm(&store_index) == SYSERR){
		return SYSERR;
	}

	//mark bs entry as mapped to this process
	bsm_tab[store_index].bs_status = BSM_MAPPED;
	bsm_tab[store_index].bs_pid    = pid;
	bsm_tab[store_index].bs_vpno   = VH_VPNO; 
	bsm_tab[store_index].bs_npages = hsize;

	//register mapping so page faults on the heap will be satisfied
    if(bsm_map(pid, VH_VPNO, store_index, hsize) == SYSERR){
		return SYSERR;
	}
	bsm_tab[store_index].bs_private = 1;// flag to mark backing store needs to be private.
	restore(ps);
	
	//initialize the process's virtual heap free list (vmemlist)
	proctab[pid].store     = store_index;
	proctab[pid].vhpno     = VH_VPNO;
	proctab[pid].vhpnpages = hsize;

	//allocate the head node 
	vhead = (struct mblock *) getmem(sizeof(struct mblock));
	if ((int)vhead == SYSERR){
		return SYSERR;
	}

	vhead->mnext = (struct mblock *)(VH_VPNO * NBPG);
	vhead->mlen = 0;
	proctab[pid].vmemlist = vhead;

	// kprintf("[vcreate] Created process %d | PD frame=%d | Store=%d | VH_VPNO=%d | Pages=%d\n", pid, frm, store_index, VH_VPNO, hsize);

	return pid;

}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
