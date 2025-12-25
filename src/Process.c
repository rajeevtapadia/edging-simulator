#include <assert.h>
#include <paging.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Proc *create_proc(char *name) {
    struct Proc *new_proc = (struct Proc *)malloc(sizeof(struct Proc));
    new_proc->pid = (size_t)new_proc & 0xFFFF;
    new_proc->name = (char *)malloc(strlen(name) + 1);
    strcpy(new_proc->name, name);
    new_proc->page_table = create_page_table(DEFAULT_PAGE_TABLE_SIZE);
    return new_proc;
}

void destroy_proc(struct Proc *proc) {
    destroy_page_table(proc->page_table);
    free(proc->name);
    free(proc);
}

uintptr_t convert_virtual_addr_to_physical_addr(struct PageTable *pt,
                                                virt_addr_t virt_addr) {
    size_t page_idx = virt_addr / PAGE_SIZE;
    assert(pt->entries[page_idx] != 0 && "[FATAL] Invalid page");

    uintptr_t frame_addr = pt->entries[page_idx];
    uintptr_t offset = virt_addr & (PAGE_SIZE - 1);
    return frame_addr + offset;
}

// Standard method to read one byte of data with virtual address
unsigned char access_memory(struct Proc *proc, virt_addr_t virt_addr) {
    assert(proc != NULL);
    assert(proc->page_table != NULL);

    struct ExecLogEntry entry = {0};
    size_t page_idx = virt_addr / PAGE_SIZE;

    // check for segmentation fault
    if (page_idx >= proc->page_table->size || proc->page_table->entries[page_idx] == 0) {
        LOG_ERROR("Page fault while accessing %p", (void *)virt_addr);
        LOG_ERROR("%s: Segmentation fault", proc->name);
        return -1;
    }

    entry.action = READ;
    entry.virt_addr = virt_addr;
    entry.proc = proc;
    push_to_exec_log(exec_log, entry);

    uintptr_t phy_addr =
        convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);
    return phy_mem[phy_addr];
}

// This methods is only supposed to be used by the visulaisation to show memory dump
// Hence it skips the logging and error handling
unsigned char inspect_memory(struct Proc *proc, virt_addr_t virt_addr) {
    assert(proc != NULL);
    assert(proc->page_table != NULL);

    size_t page_idx = virt_addr / PAGE_SIZE;

    // check for segmentation fault
    if (page_idx >= proc->page_table->size || proc->page_table->entries[page_idx] == 0) {
        return 0;
    }

    uintptr_t phy_addr =
        convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);
    return phy_mem[phy_addr];
}

void set_memory(struct Proc *proc, virt_addr_t virt_addr, unsigned char data) {
    assert(proc != NULL);
    assert(proc->page_table != NULL);

    struct ExecLogEntry entry = {0};
    size_t page_idx = virt_addr / PAGE_SIZE;

    // check for page fault
    if (proc->page_table->entries[page_idx] == 0) {
        map_frame_at_addr(proc->page_table, virt_addr);
        entry.did_map = true;
    }

    uintptr_t phy_addr =
        convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);

    // Maintain a log of the opeartion for rollback
    entry.action = WRITE;
    entry.virt_addr = virt_addr;
    entry.old_data = phy_mem[phy_addr];
    entry.new_data = data;
    entry.proc = proc;
    push_to_exec_log(exec_log, entry);

    phy_mem[phy_addr] = data;
}

bool is_proc_same(struct Proc *proc1, struct Proc *proc2) {
    return proc1->pid == proc2->pid;
}
