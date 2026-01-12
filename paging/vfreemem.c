/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];

/*------------------------------------------------------------------------
 * vfreemem - free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL vfreemem(struct mblock *block, unsigned size)
{
    STATWORD ps;
    struct mblock *p, *q;
    unsigned top;
    struct pentry *pptr;

    pptr = &proctab[currpid];

    // validating arguments 
    if(size == 0){
        return SYSERR;
    }
    if((unsigned)block <(unsigned)(pptr->vhpno *NBPG) || (unsigned)block >= (unsigned)((pptr->vhpno + pptr->vhpnpages)* NBPG)){
        return SYSERR;
    }

    size = (unsigned)roundmb(size);

    if((unsigned)block +size> (unsigned)((pptr->vhpno+pptr->vhpnpages)* NBPG)){
        return SYSERR;
    }

    disable(ps);

    // find position in the processs virtual freelist 
    for (p = pptr->vmemlist->mnext,q= pptr->vmemlist; p != (struct mblock *)NULL && p< block;
         q = p, p = p->mnext){
            // kprintf("[vfreemem] scanning: p=%x size=%u\n", p, p->mlen);
         }

    top = (unsigned)q +q->mlen;

    // check overlap or invalid plcement
    if((q != pptr->vmemlist && top >(unsigned)block) || (p != NULL && (unsigned)block + size > (unsigned)p)){
        restore(ps);
        return SYSERR;
    }

    // merge with previus block if adja
    if(q != pptr->vmemlist && top ==(unsigned)block){

        q->mlen += size;
    }

    else{
        block->mlen = size;
        block->mnext = p;
        q->mnext = block;
        q = block;
    }

    // merge with next block if adjacent
    if(p !=NULL && ((unsigned)q + q->mlen ==(unsigned)p)){
        q->mlen += p->mlen;
        q->mnext = p->mnext;
    }
    restore(ps);
    return OK;
}