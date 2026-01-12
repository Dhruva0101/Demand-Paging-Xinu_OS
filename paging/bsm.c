/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[NBS]; /* Holds the information(bs_map_t in paging) of all 8 parts of the backing store */
bs_procmap_t bsm_proc[NBS][NPROC];
/*-------------------------------------------------------------------------
 * init_bsm - initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    int i;
    for(i = 0; i<NBS; i++){
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = -1 ;
        bsm_tab[i].bs_vpno = -1;
        bsm_tab[i].bs_npages = 0 ;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].bs_private = 0;
        int j;
        for(j=0;j<NPROC;j++){
            bsm_proc[i][j].pid=-1;
            bsm_proc[i][j].vpno=-1;
            bsm_proc[i][j].npages=0;
            bsm_proc[i][j].active=0;
        }
    }
    return OK;
}/* reseting all backing store */

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab
 * 
 * out variables: avail
 *
 * If a backing store is available, the call returns OK and sets avail
 * to the index of the free backing store.
 * 
 * If no backing stores are available, return SYSERR. You may set avail
 * to any value or not set it at all, since callers should always check
 * the return code prior to making use of out variables.
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    int i;
    for(i=0;i<NBS;i++ ){
        if (bsm_tab[i].bs_status == BSM_UNMAPPED){
            *avail = i;
            return OK;
        }
    }
    return SYSERR;
}/* iterating through all the backing store to find the free one */


/*-------------------------------------------------------------------------
 * free_bsm - free backing store i in the bsm_tab
 *
 * Returns OK if the backing store was successfully freed or if the backing
 * store was already free.
 * 
 * Returns SYSERR if the backing store could not be successfully freed.
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    if (0<=i &&i<NBS){
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = -1 ;
        bsm_tab[i].bs_vpno = -1;
        bsm_tab[i].bs_npages = 0 ;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].bs_private=0;
        int j;
        for(j=0;j<NPROC;j++){
            bsm_proc[i][j].pid=-1;
            bsm_proc[i][j].vpno=-1;
            bsm_proc[i][j].npages=0;
            bsm_proc[i][j].active=0;
        }
        return OK;
    }
    else{
        return SYSERR;
    }
}/* Empty the required backing store */

/*-------------------------------------------------------------------------
 * bsm_lookup - find the backing store and page corresponding to the given
 * pid and vaddr.
 * 
 * out variables: store, pageth
 * 
 * If there is a corresponding backing store and the vaddr is valid,
 * return OK, set store to the backing store index, and pageth to the 
 * relative page number within the backing store.
 * 
 * If no mapping exists or the vaddr is invalid, return SYSERR. You
 * may set store and pageth to any value or not set them at all, since 
 * callers should always check the return code prior to making use of 
 * out variables.
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    unsigned long vpno = (unsigned)vaddr>>12;
    int i;
    for(i=0;i<NBS ; i++){
        if (bsm_tab[i].bs_status == BSM_MAPPED /* status chekc */){
        int j;
        for(j=0;j<NPROC;j++){
            if(bsm_proc[i][j].active && bsm_proc[i][j].pid==pid
            && vpno>=bsm_proc[i][j].vpno&& +vpno<(bsm_proc[i][j].vpno +bsm_proc[i][j].npages)){
                *store = i;
                *pageth = vpno - bsm_proc[i][j].vpno; /* vpgno. - vpg start of BS */
                return OK;
            }
            }
        }
    }
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add a mapping into bsm_tab for the given pid starting at the
 * vpno.
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
        if (source>=0 && source<NBS && npages>0 && npages<=256 &&bsm_tab[source].bs_status == BSM_UNMAPPED){/* check for correct inputs */
        bsm_tab[source].bs_status = BSM_MAPPED;
        bsm_tab[source].bs_pid = pid ;
        bsm_tab[source].bs_vpno = vpno;
        bsm_tab[source].bs_npages = npages ;
        bsm_tab[source].bs_sem = 0;
        bsm_tab[source].bs_private= (vpno ==VH_VPNO);
        }/* Setting the BS to the input if conditions satisfy */
        int j;
        for(j=0;j<NPROC;j++){
            if(bsm_proc[source][j].active==0){
            bsm_proc[source][j].pid=pid;
            bsm_proc[source][j].vpno=vpno;
            bsm_proc[source][j].npages=npages;
            bsm_proc[source][j].active=1;
            return OK;
            }
        }
        return SYSERR;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete the mapping from bsm_tab that corresponds to the 
 * given pid, vpno pair.
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno)
{
    int i; 
    for(i = 0; i<NBS ; i++){
        if(bsm_tab[i].bs_status == BSM_MAPPED){/* checks for matching input */
            int j;
            for(j=0;j<NPROC;j++){
                if(bsm_proc[i][j].active&& bsm_proc[i][j].pid==pid&& bsm_proc[i][j].vpno==vpno){
                    int priv = bsm_tab[i].bs_private;
                    bsm_proc[i][j].active=0;
                    if(priv == 1){
                        free_bsm(i);
                        return OK;
                    }
                    int inuse =0;
                    int k;
                    for(k=0;NPROC;k++){
                        if(bsm_proc[i][k].active==1){
                            inuse = 1;
                            break;
                        }
                    }
                    if(inuse == 0){
                        free_bsm(i);
                    }
                    return OK;
                }
            }
        }/* reset the BS when conditions satisfy */
    }
    return SYSERR;
}
