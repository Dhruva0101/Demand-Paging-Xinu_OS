/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
extern int pr_debug;
/*-------------------------------------------------------------------------
 * enable_pr_debugging - turn on page replacement debug logging.
 *-------------------------------------------------------------------------
 */
SYSCALL enable_pr_debug()
{
  /* sanity check ! */

  pr_debug = 1;

  return OK;
}

/*-------------------------------------------------------------------------
 * disable_pr_debugging - turn off page replacement debug logging.
 *-------------------------------------------------------------------------
 */
SYSCALL disable_pr_debug()
{
  /* sanity check ! */

  pr_debug = 0;

  return OK;
}