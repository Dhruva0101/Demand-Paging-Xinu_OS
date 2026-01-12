#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * read_bs - read the specified page from the backing store corresponding 
 * to bs_id into dst.
 * 
 * out variables: dst
 *-------------------------------------------------------------------------
 */
SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {

  /* fetch page page from map map_id
     and write beginning at dst.
  */
  if(bs_id>=0 && bs_id<NBS && page>=0 && page<256){
   void * phy_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + page*NBPG;/* value of phy addr*/
   bcopy(phy_addr, (void*)dst, NBPG);
   return OK;
  }
  return SYSERR;
}


