#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>

#define NUM_GLOBAL_PT 4      // 4 global page tables

extern fr_map_t frm_tab[];
pt_t *global_pt[NUM_GLOBAL_PT];   // array of 4 pointers, one per global page table

//----------------------------------------------------------------------------------------------------
// Create 4 global page tables
//----------------------------------------------------------------------------------------------------

static void init_global_page_tables(void) {

    unsigned int i,j;          // i → which page table, j → which entry inside that table
    unsigned long phys_addr;   // physical address that each entry will point to

        // Loop all 4 global page tables
        for (i = 0; i < NUM_GLOBAL_PT; i++){

            // location in memory where each PT begins - FRAME0 * NBPG = 4MB → first global PT starts there
            global_pt[i] = (pt_t *)(FRAME0 * NBPG + i  * NBPG);

            // fill all 1024 entries in each PT
            for(j = 0; j < NFRAMES; j++){

                // set flags for this page table entry
                phys_addr = (i * NFRAMES + j) *NBPG;

                // set flags for this page table entry
                global_pt[i][j].pt_pres   =1; // page is present
                global_pt[i][j].pt_write  =1; // writable = 1, read-only = 0
                global_pt[i][j].pt_user   =0; // kernel mode =0 , user mode = 1
                global_pt[i][j].pt_pwt    =0; // write-through caching = 0 (disabled)
                global_pt[i][j].pt_pcd    =0; // cache disable = 0 (cache enabled)
                global_pt[i][j].pt_acc    =0; // accessed = 0 (not used yet)
                global_pt[i][j].pt_dirty  =0; // written-to = 0
                global_pt[i][j].pt_mbz    =0; // must be zero (hardware-reserved)
                global_pt[i][j].pt_global =0; // global mapping = 0 (not global)
                global_pt[i][j].pt_avail  =0; // available bits = 0 (unused)
                global_pt[i][j].pt_base   = (phys_addr >>12);  // frame number = phys_addr / 4KB

            }
        }
        // kprintf("[paginginit] 4 global page tables created successfully\n"); // testing
    }

//----------------------------------------------------------------------------------------------------
// Create Page Directory for NULL process
//----------------------------------------------------------------------------------------------------

static pd_t* init_null_process_pd(void) {

    unsigned int i;
    pd_t *pd; // pointer to page directory structure

    // each pde -> one pt,pt we already used frames 1024–1027 for 4 PTs, PD starts at frame 1028 → (1024+4)*4096 = 0x00404000 physical addr
    pd = (pd_t *)((FRAME0 + NUM_GLOBAL_PT) * NBPG); 
        
    // kprintf("[paginginit] Page Directory for NULL process at frame: %d (phys addr : 0x%08x)\n", 
    //     FRAME0 + NUM_GLOBAL_PT, (FRAME0 + NUM_GLOBAL_PT) * NBPG); // testing

        //Link PD entries 0–3 to the 4 global PTs
        for(i =0 ; i < NUM_GLOBAL_PT; i++){

            pd[i].pd_pres     =1;  // page table is present
            pd[i].pd_write    =1;  // writable = 1, read-only = 0
            pd[i].pd_user     =0;  // kernel mode =0 , user mode = 1
            pd[i].pd_pwt      =0;  // write-through caching = 0 (disabled)
            pd[i].pd_pcd      =0;  // cache disable = 0 (cache enabled)
            pd[i].pd_acc      =0;  // accessed = 0 (not used yet)
            pd[i].pd_mbz      =0;  // must be zero
            pd[i].pd_fmb      =0;  // 4MB page = 1 else 0
            pd[i].pd_global   =0;  // global mapping = 0 (not global)
            pd[i].pd_avail    =0;  // available bits = 0 (unused)
            pd[i].pd_base     =((unsigned long)global_pt[i]) >> 12;  // physical frame number of PT
            // shift right by 12 = divide by 4096 = get page frame index

            // kprintf("[paginginit] PD entry %d → PT frame base: 0x%05x\n", i, pd[i].pd_base); //testing
        }

        // mark remaining 1020 PD entries as not present
        for(i= NUM_GLOBAL_PT ; i < NFRAMES ; i++){
            pd[i].pd_pres = 0;  // mark as not present, any access will trigger page fault
        }
        // kprintf("[paginginit] PD entries 0-3 mapped, 4-1023 not present\n");  // testing
        return pd;
    }

    // Load CR3 and enable paging

    static void enable_paging(pd_t *pd) {
    unsigned long cr0;

    // Save PD base for null process
    proctab[NULLPROC].pdbr = (unsigned long)pd;

    // Load PD base into CR3 (physical address)
    write_cr3((unsigned long)pd);
    // kprintf("[paginginit] CR3 loaded with PD @ 0x%08x\n",(unsigned long)read_cr3()); // testing

    // Enable paging (set PG bit = bit 31 in CR0)
    cr0 = read_cr0();
    write_cr0(cr0 | 0x80000000);
    // kprintf("[paginginit] CR0.PG set -> paging ENABLED\n"); // testing
}

//----------------------------------------------------------------------------------------------------
// paginginit() — builds 4 global page tables mapping 0–16MB of physical memory with identity mapping
// ---------------------------------------------------------------------------------------------------

void paginginit(void){

    // kprintf("\n[paginginit] Paging initialization started...\n"); //testing -DK

    init_frm();
    init_global_page_tables();
    pd_t *pd = init_null_process_pd();

    int i;
    for (i = 0; i < 5; i++) {                 // indices 0..3 => FRAME0+0 .. FRAME0+3
        frm_tab[i].fr_status = FRM_MAPPED;
        frm_tab[i].fr_type   = (i < 4) ? FR_TBL : FR_DIR;
        frm_tab[i].fr_pid    = NULLPROC;         // owned by the system / null proc
        frm_tab[i].fr_vpno   = -1;
        frm_tab[i].fr_refcnt = (i < 4) ? NFRAMES : 1;            // each PT maps NFRAMES pages
        frm_tab[i].fr_dirty  = 0;
    }

    // kprintf("[paginginit] Reserved PT frames 0-3 and PD frame 4 in frm_tab\n");
    srpolicy(SC);//changed
    enable_paging(pd);

    extern void pfintr(void);
    set_evec(14,(u_long)pfintr);  //PAGEFAULT HANDLER
}