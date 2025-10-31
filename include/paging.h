/*
 * Entities:
 *    Process
 *    Main/Physical memory
 *    Virtual memory
 *    Page table: map of p:f
 *    Frames(f): 4KB block of main memory
 *    Page(p)
 *
 * Assumptions:
 *    64-bit addressing space
 */

#include <assert.h>
#include <stdint.h>

#define FRAME_SIZE 4 * 1024
#define PAGE_SIZE FRAME_SIZE
static_assert((FRAME_SIZE & (FRAME_SIZE - 1)) == 0 && "FRAME_SIZE should be a power of 2\n");

/*
 * 0b0000000000000000000000000000000000000000000000000000 000000000000
 *   |           FRAME/PAGE_ADDR_BITS                    |OFFSET_BITS|
 */
#define OFFSET_BITS __builtin_ctz(FRAME_SIZE)
#define FRAME_ADDR_BITS 64 - OFFSET_BITS
#define PAGE_ADDR_BITS 64 - OFFSET_BITS

// Buffer that acts as physical memory
extern unsigned char *phy_mem;
/*
 * Index is page number p
 * Value at index p is frame address
 */
extern unsigned char *page_table;

struct Process {};
