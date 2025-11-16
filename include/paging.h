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
#ifndef PAGING_H
#define PAGING_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FRAME_SIZE (4 * 1024)
#define PAGE_SIZE FRAME_SIZE
static_assert((FRAME_SIZE & (FRAME_SIZE - 1)) == 0,
              "FRAME_SIZE should be a power of 2\n");
static_assert(sizeof(uintptr_t) == 8, "[Error] Not 64 bit arch");

/*
 * 0b0000000000000000000000000000000000000000000000000000 000000000000
 *   |           FRAME/PAGE_ADDR_BITS                    |OFFSET_BITS|
 */
#define OFFSET_BITS __builtin_ctz(FRAME_SIZE)
#define FRAME_ADDR_BITS 64 - OFFSET_BITS
#define PAGE_ADDR_BITS 64 - OFFSET_BITS

#define DEFAULT_MEMORY_SIZE (1024 * 1024) // 1MB
#define DEFAULT_PAGE_TABLE_SIZE DEFAULT_MEMORY_SIZE / FRAME_SIZE

#define LOG_INFO(fmt, ...) fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

typedef uintptr_t virt_addr_t;

struct PageTable {
    uintptr_t *entries;
    size_t size;
    size_t curr;
};

struct Proc {
    char* name;
    struct PageTable *page_table;
};

extern unsigned char *phy_mem;
extern size_t last_frame_id;

// Process.c
struct Proc *create_proc(char *name);
void destroy_proc(struct Proc *proc);
void set_memory(struct Proc *proc, virt_addr_t virt_addr, unsigned char data);
unsigned char access_memory(struct Proc *proc, virt_addr_t virt_addr);

// PageTable.c
struct PageTable *create_page_table(size_t size);
void destroy_page_table(struct PageTable *pt);
void print_page_table(struct PageTable *pt);
void map_frame_at_addr(struct PageTable *page_table, virt_addr_t virt_addr);

#endif // PAGING_H
