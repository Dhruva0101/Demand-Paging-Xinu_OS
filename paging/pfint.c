/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/* Reading the CR2 register to get the virtual address of the fault. */
extern unsigned long read_cr2();

static int store_map[NFRAMES];
static int page_map[NFRAMES];
static int map_inited=0;
static void init_shared_map(){
    if(map_inited){
        return;
    }
    int i;
    for(i=0 ; i<NFRAMES; i++){
        store_map[i] =-1;
        page_map[i]=-1;
    }
    map_inited=1;
}
/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  unsigned long faulted_addr = read_cr2();
  unsigned long vpno =faulted_addr/NBPG;
  int store;
  int pageth;
  int bsm_return;
  bsm_return = bsm_lookup(currpid,faulted_addr, &store, &pageth);
  if (bsm_return== SYSERR){
      kill(currpid);
      return SYSERR;
    }
  virt_addr_t *vat =(virt_addr_t*)&faulted_addr;/* Created a pointer over the vaddr to get pt/pd indices */
  int pdi = vat->pd_offset;/* Gets the page directory index */
  int pti = vat->pt_offset;/* Gets the page table index */
  pd_t *pd =(pd_t*)(proctab[currpid].pdbr);
  pd_t *pde =&pd[pdi];
  if (pde->pd_pres==0){
    int pftb_frno;/* free frame available  */
    int frm_return;
    frm_return = get_frm(&pftb_frno);
    if (frm_return== SYSERR){
      kprintf("[pfint] No free frame available\n");
      return SYSERR;
    }
    pt_t *npgtb =(pt_t*)((FRAME0 + pftb_frno) * NBPG);
    int i;
    for(i = 0; i<1024; i++){
      npgtb[i].pt_pres	=0;		
      npgtb[i].pt_write =1;		
      npgtb[i].pt_user	=0;		
      npgtb[i].pt_pwt	=0;		
      npgtb[i].pt_pcd	=0;		
      npgtb[i].pt_acc	=0;
      npgtb[i].pt_dirty =0;	
      npgtb[i].pt_mbz	=0;		
      npgtb[i].pt_global=0;	
      npgtb[i].pt_avail =0;
      npgtb[i].pt_base	=0;
    }

    frm_tab[pftb_frno].fr_status = FRM_MAPPED;
    frm_tab[pftb_frno].fr_pid    = currpid;
    frm_tab[pftb_frno].fr_vpno   = 0;
    frm_tab[pftb_frno].fr_refcnt = 0;
    frm_tab[pftb_frno].fr_type   = FR_TBL;
    frm_tab[pftb_frno].fr_dirty  = 0;
    
    pde->pd_pres     =1;  
    pde->pd_write    =1;  
    pde->pd_user     =0;  
    pde->pd_pwt      =0;  
    pde->pd_pcd      =0;  
    pde->pd_acc      =0;  
    pde->pd_mbz      =0;  
    pde->pd_fmb      =0;  
    pde->pd_global   =0;  
    pde->pd_avail    =0;  
    pde->pd_base     =FRAME0 +pftb_frno;  
  }
  pt_t *pgtb = (pt_t*)((pde->pd_base) * NBPG);//matching till here

  int existing_frno = -1;
  int i;
  for(i=0;i<NFRAMES;i++){
    if(frm_tab[i].fr_status==FRM_MAPPED &&frm_tab[i].fr_type==FR_PAGE
    && store_map[i]==store && page_map[i]== pageth ){
        existing_frno = i;
        break;
    }
  }
  int pftb_frno;/* free frame available  */
  int frm_return;
  if(existing_frno!= -1){
    pftb_frno= existing_frno;
    frm_tab[pftb_frno].fr_refcnt=frm_tab[pftb_frno].fr_refcnt+1;
  }
  else{
    frm_return = get_frm(&pftb_frno);
    if (frm_return== SYSERR){
        return SYSERR;
    }
    char *f_addr = (char*)((FRAME0 + pftb_frno) * NBPG);
    bsm_return = read_bs(f_addr,store,pageth);
    if (bsm_return==SYSERR){
        free_frm(pftb_frno);
        return SYSERR;
    }
      frm_tab[pftb_frno].fr_status= FRM_MAPPED;
      frm_tab[pftb_frno].fr_pid = currpid;
      frm_tab[pftb_frno].fr_vpno = vpno;
      frm_tab[pftb_frno].fr_refcnt = 1;
      frm_tab[pftb_frno].fr_type = FR_PAGE;
      frm_tab[pftb_frno].fr_dirty  =0;
      store_map[pftb_frno] = store;
      page_map[pftb_frno]= pageth;
    }
  pgtb[pti].pt_pres	=1;		
  pgtb[pti].pt_write =1;		
  pgtb[pti].pt_user	=0;		
  pgtb[pti].pt_pwt	=0;		
  pgtb[pti].pt_pcd	=0;		
  pgtb[pti].pt_acc	=0;
  pgtb[pti].pt_dirty =0;	
  pgtb[pti].pt_mbz	=0;		
  pgtb[pti].pt_global=0;	
  pgtb[pti].pt_avail =0;
  pgtb[pti].pt_base	=FRAME0 + pftb_frno;

  int frm_index=pde->pd_base - FRAME0;
  if (frm_index < NFRAMES && frm_index>=0){
    frm_tab[frm_index].fr_refcnt = frm_tab[frm_index].fr_refcnt +1;
  }
  extern unsigned long read_cr3(void);
  extern void write_cr3(unsigned long);
  write_cr3(read_cr3());   /* TLB flush so CPU sees the new PT */
  return OK;
}