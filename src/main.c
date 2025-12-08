#include <paging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(DEFAULT_MEMORY_SIZE % FRAME_SIZE == 0,
              "DEFAULT_MEMORY_SIZE size should be divisible by FRAME_SIZE");

unsigned char *phy_mem = NULL;
size_t last_frame_id = 0;

int main() {
    LOG_INFO("arch: %d bit", 8 * (int)sizeof(uintptr_t));

    phy_mem = malloc(DEFAULT_MEMORY_SIZE);

    LOG_INFO("default table size: %d pages", DEFAULT_PAGE_TABLE_SIZE);
    struct Proc *proc1 = create_proc("proc 1");
    struct Proc *proc2 = create_proc("proc 2");

    multi_process_visualisation(proc1, proc2);

    destroy_proc(proc1);
    destroy_proc(proc2);
    free(phy_mem);

    return 0;
}
