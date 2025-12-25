#include <paging.h>
#include <raylib.h>
#include <simulator-ui.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Proc *proc1 = NULL;
static struct Proc *proc2 = NULL;

static void next_operation_handler() {
    if (IsKeyReleased(KEY_N) &&
        test_case.curr_operation_idx < test_case.operation_count) {
        print_operation(&test_case.ops[test_case.curr_operation_idx]);
        perform_operation(&test_case.ops[test_case.curr_operation_idx]);
        test_case.curr_operation_idx++;
    }
    if (IsKeyReleased(KEY_P)) {
        roll_back_opearation(exec_log);
    }

    if (IsKeyReleased(KEY_SPACE) && focus.is_selected) {
        struct PageTable *selected_pt = focus.proc->page_table;

        // decide if page will be mapped or unmapped
        if (selected_pt->entries[focus.page_table_idx] == 0) {
            set_memory(focus.proc, focus.page_table_idx << 12, 0xFF);
        } else {
            unmap_page_by_page_idx(selected_pt, focus.page_table_idx);
        }
    }

    if (IsKeyReleased(KEY_L)) {
        print_exec_stack(exec_log);
    }
}

static struct Proc *proc_at_cursor() {
    float mouse_x = GetMouseX();
    return (mouse_x < GetScreenWidth() / 2.f ? proc1 : proc2);
}

static void mouse_click_handler() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int idx = page_table_idx_at_cursor();
        if (idx == -1) {
            return;
        }
        if (idx == 0) {
            LOG_WARN("Mapping guard page not allowed");
            return;
        }
        struct Proc *proc = proc_at_cursor();
        if (focus.is_selected && is_proc_same(focus.proc, proc) &&
            focus.page_table_idx == (size_t)idx) {
            focus.is_selected = false;
        } else {
            focus.page_table_idx = idx;
            focus.proc = proc;
            focus.is_selected = true;
        }
    }
}

static void render_loop() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawText("Multi-Process Physical Memory Access", 380, GetScreenWidth() / 64, 40,
                 TITLE_COLOR);

        next_operation_handler();

        size_t left_padding = LEFT_PADDING;
        size_t right_padding = GetScreenWidth() - left_padding - BOX_WIDTH;
        draw_page_table(proc1, left_padding);
        draw_page_table(proc2, right_padding);
        draw_physical_memory();

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc1->page_table->entries[i] != 0) {
                size_t frame_idx = proc1->page_table->entries[i] >> 12;
                draw_arrow_from_proc_left(i, frame_idx);
            }
        }

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc2->page_table->entries[i] != 0) {
                size_t frame_idx = proc2->page_table->entries[i] >> 12;
                draw_arrow_from_proc_right(i, frame_idx);
            }
        }

        draw_divider();
        draw_text_section();

        mouse_click_handler();

        EndDrawing();
    }
}

static void init_visualsation() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Paging Simulator");

    render_loop();

    CloseWindow();
}

static void create_test_case_1() {
    test_case.curr_operation_idx = 0;

    test_case.ops = (struct Operation *)malloc(sizeof(struct Operation) * 20);

    int i = 0;

    // proc1 writes to pages (causing page faults)
    test_case.ops[i++] = (struct Operation){
        .action = WRITE, .proc = proc1, .data = 'A', .virt_addr = 0x1000};
    test_case.ops[i++] = (struct Operation){
        .action = WRITE, .proc = proc1, .data = 'B', .virt_addr = 0x2000};

    // proc1 reads what it wrote
    test_case.ops[i++] =
        (struct Operation){.action = READ, .proc = proc1, .virt_addr = 0x1000};
    test_case.ops[i++] =
        (struct Operation){.action = READ, .proc = proc1, .virt_addr = 0x2000};

    // proc2 creates independent pages
    test_case.ops[i++] = (struct Operation){
        .action = WRITE, .proc = proc2, .data = 'X', .virt_addr = 0x1000};
    test_case.ops[i++] = (struct Operation){
        .action = WRITE, .proc = proc2, .data = 'Y', .virt_addr = 0x4000};

    // proc2 reads
    test_case.ops[i++] =
        (struct Operation){.action = READ, .proc = proc2, .virt_addr = 0x1000};

    // Switch back to proc1 - check page table isolation
    test_case.ops[i++] =
        (struct Operation){.action = READ, .proc = proc1, .virt_addr = 0x1000};

    // Two processes touching the same virtual address (0x3000)
    test_case.ops[i++] = (struct Operation){
        .action = WRITE, .proc = proc1, .data = 'C', .virt_addr = 0x3000};
    test_case.ops[i++] = (struct Operation){
        .action = WRITE, .proc = proc2, .data = 'Z', .virt_addr = 0x3000};
    test_case.ops[i++] =
        (struct Operation){.action = READ, .proc = proc1, .virt_addr = 0x3000};
    test_case.ops[i++] =
        (struct Operation){.action = READ, .proc = proc2, .virt_addr = 0x3000};

    test_case.ops[i++] =
        (struct Operation){.action = UNMAP, .proc = proc1, .virt_addr = 0x1000};
    test_case.ops[i++] =
        (struct Operation){.action = UNMAP, .proc = proc1, .virt_addr = 0x2000};

    test_case.operation_count = i;
}

void multi_process_visualisation(struct Proc *_proc1, struct Proc *_proc2) {
    proc1 = _proc1;
    proc2 = _proc2;
    proc1->page_table->size = sim_page_size;
    proc2->page_table->size = sim_page_size;

    create_test_case_1();

    init_visualsation();

    free(test_case.ops);
}
