/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - map the virtual page to the backing store source with a 
 * permitted access range of npages (<= 256) for the calling process.
 * 
 * Return OK if the call succeeded and SYSERR if it failed for any reason.
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  int bsm_return;
  if (virtpage>=4096 && source>=0 && source < NBS && npages < 256 && npages > 0){
    if(proctab[currpid].pdbr==0){
        int free_frame;
        if (get_frm(&free_frame)== SYSERR){
            return SYSERR;
        }
        frm_tab[free_frame].fr_status= FRM_MAPPED;
        frm_tab[free_frame].fr_pid= currpid;
        frm_tab[free_frame].fr_vpno= -1;
        frm_tab[free_frame].fr_refcnt= 1;
        frm_tab[free_frame].fr_type= FR_DIR;
        frm_tab[free_frame].fr_dirty= 0;

        unsigned long physical_addr_pgdr;
        physical_addr_pgdr = (FRAME0 + free_frame)*NBPG;
        pd_t *pgdr_ptr ;
        pgdr_ptr = (pd_t*)physical_addr_pgdr ;
        int i;
        for(i=0;i<1024;i++){
            pgdr_ptr[i]= (pd_t){0};
        }
        for(i=0;i<4;i++){
            pgdr_ptr[i].pd_pres=1;
            pgdr_ptr[i].pd_write=1;
            pgdr_ptr[i].pd_user=0;
            pgdr_ptr[i].pd_base=((unsigned long)global_pt[i]) >> 12;
        }
        proctab[currpid].pdbr = physical_addr_pgdr;
    }
    bsm_return = bsm_map(currpid, virtpage, source, npages);
    if (bsm_return== OK){
      return OK;
    }
    else{
      return SYSERR;
    }
  }
  return SYSERR;
}

/*-------------------------------------------------------------------------
 * xmunmap - free the xmmapping corresponding to virtpage for the calling 
 * process.
 * 
 * This call should not affect any other processes' xmmappings.
 * 
 * Return OK if the unmapping succeeded. Return SYSERR if the unmapping
 * failed or virtpage did not correspond to an existing xmmapping for
 * the calling process.
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  int bsm_return;
  if (virtpage>=4096){
    bsm_return = bsm_unmap(currpid, virtpage);
    if (bsm_return== OK){
      return OK;
    }
    else{
      return SYSERR;
    }
  }
  return SYSERR;
}