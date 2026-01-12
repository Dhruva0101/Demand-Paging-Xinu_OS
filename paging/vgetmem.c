/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem - allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{	

	STATWORD ps;
	struct pentry *pptr;
	struct	mblock	*p, *q;

	pptr =&proctab[currpid];
	
	//validate arguments
	if(nbytes == 0){
		return (WORD *)SYSERR;
	}

    nbytes = (unsigned)roundmb(nbytes);

	disable(ps);

// 	kprintf("[vgetmem] pid=%d vmemlist=%x mnext=%x\n", currpid, pptr->vmemlist, pptr->vmemlist->mnext);
// 	if (pptr->vmemlist->mnext)
//     kprintf("[vgetmem] first block len=%u next=%x\n",
//             pptr->vmemlist->mnext->mlen, pptr->vmemlist->mnext->mnext);

    if(pptr ->vmemlist == NULL){
            restore(ps);
            return (WORD *)SYSERR;
    }

    q = pptr->vmemlist;
    p = q->mnext;

    if(p == (struct mblock *)(pptr->vhpno *NBPG)){
        
        if(p == NULL || p->mlen ==0 || p->mlen > pptr->vhpnpages* NBPG){
            p = (struct mblock *)(pptr->vhpno *NBPG);
            p->mlen = pptr->vhpnpages * NBPG;
            p->mnext = NULL;
            q->mnext = p;
        }
    }

	for(q = pptr->vmemlist,p = pptr->vmemlist->mnext; p != (struct mblock *)NULL;q = p, p = p->mnext){
		
		if (p->mlen == nbytes){
		q->mnext =p->mnext;

		restore(ps);
		return (WORD *)p;
	    }
		else if(p->mlen > nbytes){
			struct mblock *leftover;
			leftover = (struct mblock *)((unsigned)p + nbytes);
			leftover->mlen = p->mlen - nbytes;
			leftover->mnext = p->mnext;
			q->mnext = leftover;
			
			restore(ps);
	        return (WORD *)p;
		}
	}
	restore(ps);
	return (WORD *)SYSERR;
}
