/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES]; 

extern int page_replace_policy;
int pr_debug = 0;

// Circular queue implmetnation for second chance rp

static int fr_queue[NFRAMES];
static int q_head = -1;
static int q_tail = -1;
static int q_count = 0;

// ---------------------------------------------------------------------------------------------------
// Add a frame to the SC queue
// ---------------------------------------------------------------------------------------------------

void enqueue_frm(int f){

  if(q_count == NFRAMES){
    return;
  }
  if(q_head == -1 ){
    q_head = 0;
  }
  q_tail = (q_tail +1) % NFRAMES;
  fr_queue[q_tail] = f;
  q_count++;
}

// ---------------------------------------------------------------------------------------------------
// Remove oldest frame from queue
// ---------------------------------------------------------------------------------------------------

int dequeue_frm(void){

  if (q_count == 0 ){
    return SYSERR;
  }

  int fr = fr_queue[q_head];
  q_head = (q_head +1) % NFRAMES;
  q_count--;

  if(q_count == 0){
    q_head = -1;
    q_tail = -1;
  }
  return fr;
}

// ---------------------------------------------------------------------------------------------------
// Second chance page replacement policy
// ---------------------------------------------------------------------------------------------------

int get_fr_replace(void){

  if (q_count == 0){
    return SYSERR;
  }

  int visited = 0;

  while(visited < NFRAMES){

    int frm = fr_queue[q_head];

    if (frm < 0 || frm >= NFRAMES || frm_tab[frm].fr_pid < 0){

        int victim_frm = dequeue_frm();

        // if(pr_debug){
        //   kprintf("[SC] Evict frame %d (invalid pid)\n", victim_frm);
        // }
      return victim_frm;
    }
    
    pt_t *pt;
    pd_t *pd; 
    
    //getting frame metadata
    int pid = frm_tab[frm].fr_pid;
    
    unsigned long vaddr = frm_tab[frm].fr_vpno * NBPG;
    unsigned long q = (vaddr >> 12) & 0x3FF;   /* pt index */
    unsigned long p = vaddr >> 22;             /* pd index */

    pd =(pd_t *)(proctab[pid].pdbr + (p * sizeof(pd_t)));

    if (pd->pd_pres == 1){

      pt = (pt_t *)((pd->pd_base) * NBPG);

      if(pt[q].pt_acc == 0){

          int victim_frm = dequeue_frm();

          // if(pr_debug){
          //   kprintf("[SC] Evict frame %d\n",victim_frm);
          // }
          return victim_frm;
      }
      pt[q].pt_acc = 0;
      enqueue_frm(dequeue_frm());
    }

    else{

      int victim_frm = dequeue_frm();
      // if(pr_debug){
      //   kprintf("[SC] Evict frame %d (not present)\n",victim_frm);
      // }
      return victim_frm;
      }

    visited++;
  }
  return dequeue_frm();
}

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab (the inverted page table)
 *-------------------------------------------------------------------------
 */
//Initialize the entire frm_tab[] to a clean, unmapped state,called once during system boot (in paginginit() or sysinit())
SYSCALL init_frm()
{
  int i;
  for (i =0; i < NFRAMES; i++){
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_pid    = -1;
    frm_tab[i].fr_vpno   = -1;
    frm_tab[i].fr_refcnt = 0;
    frm_tab[i].fr_type   = -1;
    frm_tab[i].fr_dirty  = 0;
  }
  // kprintf("[init_frm] Frame table initialized (%d frames)\n",NFRAMES);  //testing
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 * 
 * out variables: avail
 * 
 * If a frame is available, return OK and set avail to the available frame.
 * 
 * If no frames are available, return SYSERR and set avail to any value or 
 * not set it at all, since callers should always check the return code prior 
 * to making use of out variables.
 *-------------------------------------------------------------------------
 */

 //Allocate a free physical frame for any use (page, PT, or PD)
SYSCALL get_frm(int* avail)
{
  int i;
  //find free frame
  for (i = 0; i < NFRAMES; i++){ //)))))
    if(frm_tab[i].fr_status == FRM_UNMAPPED){
      frm_tab[i].fr_status = FRM_MAPPED;
      *avail = i;
      // kprintf("[get_frm] Allocated frame %d\n", i);
      return OK;
    }
  }

  //run sc replacement policy
  int victim_frm = get_fr_replace();
  if(victim_frm == SYSERR){
    // kprintf("[get_frm] No frame available for replacement.\n");
    return SYSERR;
  }
  *avail = victim_frm;
  enqueue_frm(victim_frm);
  // if(pr_debug){
  //   kprintf("[get_frm] Replaced frame %d\n",victim_frm);
  // }
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free frame i
 *-------------------------------------------------------------------------
 */

 //Reset a frame’s metadata to “unmapped” so it can be reused
SYSCALL free_frm(int i)
{
  if (i < 0 || i >= NFRAMES){
    return SYSERR;
  }

  frm_tab[i].fr_status = FRM_UNMAPPED;
  frm_tab[i].fr_pid    = -1;
  frm_tab[i].fr_vpno   = -1;
  frm_tab[i].fr_refcnt = 0;
  frm_tab[i].fr_type   = -1;
  frm_tab[i].fr_dirty  = 0;

  // kprintf("[free_frm] Freed frame %d\n", i);
  return OK;
}


/*------------------------------------------------------------
 * srpolicy  --  set the page replacement policy (FIFO or SC)
 *------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
    if (policy != AGING && policy != SC){
        return SYSERR;
    }

    page_replace_policy = policy;

    // if (policy == SC){
    //     kprintf("[srpolicy] Set policy = Second Chance\n");
    // }
    // else if (policy == AGING && pr_debug){
    //     kprintf("[srpolicy] Set policy = FIFO\n");
    // }
    return OK;
}

/*------------------------------------------------------------
 * grpolicy  --  get the current page replacement policy
 *------------------------------------------------------------
 */
SYSCALL grpolicy()
{
    return page_replace_policy;
}