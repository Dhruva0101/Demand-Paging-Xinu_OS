/* paging.h */

typedef unsigned int	 bsd_t;

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct{
  int bs_status;			/* MAPPED or UNMAPPED		*/
  int bs_pid;				/* process id using this slot   */
  int bs_vpno;				/* starting virtual page number */
  int bs_npages;			/* number of pages in the store */
  int bs_sem;				/* semaphore mechanism ?	*/
  int bs_private;
} bs_map_t;

typedef struct {
    int pid;
    int vpno;
    int npages;
    int active;
} bs_procmap_t;

typedef struct{
  int fr_status;			/* MAPPED or UNMAPPED		*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count		*/
  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  int fr_dirty;
}fr_map_t;

extern pt_t *global_pt[];
// the backing store mappings of the currently active process
extern bs_map_t bsm_tab[];
// the inverted page table used for page allocation and replacement
extern fr_map_t frm_tab[];
/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xmunmap(int);

/* given calls for dealing with backing store */

SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);

SYSCALL get_frm(int *avail);
SYSCALL get_bsm(int *avail);
SYSCALL bsm_map(int pid,int vpno,int source,int npages);

SYSCALL init_frm(void);
SYSCALL free_frm(int i);
SYSCALL srpolicy(int policy);
SYSCALL grpolicy(void);

int get_fr_replace(void);
void enqueue_frm(int f);
int dequeue_frm(void);

extern int page_replace_policy;
extern int pr_debug;

WORD *vgetmem(unsigned nbytes);
SYSCALL vfreemem(struct mblock *block, unsigned size);
SYSCALL xmunmap(int virtpage);
SYSCALL shutdown(void);

#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/
#define NFRAMES 	1024	/* number of frames		*/
#define NBS 8 /* om (number of backing store division) */
#define BSM_UNMAPPED	0
#define BSM_MAPPED	1

#define VH_VPNO 4096 // Base VPNO for private heap

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define SC 3
#define AGING 4

#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000

extern bs_procmap_t bsm_proc[NBS][NPROC];
extern fr_map_t     frm_tab[NFRAMES];


