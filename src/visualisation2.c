#include <ctype.h>
#include <paging.h>
#include <raylib.h>
#include <simulator-ui.h>
#include <stdio.h>

static int viewport_offset = 256;

static struct Proc *proc = NULL;

static void keyboard_handler() {
    // if (IsKeyReleased(KEY_N) &&
    // test_case.curr_operation_idx < test_case.operation_count) {
    // print_operation(&test_case.ops[test_case.curr_operation_idx]);
    // perform_operation(&test_case.ops[test_case.curr_operation_idx]);
    // test_case.curr_operation_idx++;
    // }
    // if (IsKeyReleased(KEY_P)) {
    //     roll_back_opearation(exec_log);
    // }
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
    return GetMouseX() < GetScreenWidth() / 2.f ? proc : NULL;
}

static void mouse_click_handler() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (proc_at_cursor() == NULL) {
            return;
        }

        int idx = page_table_idx_at_cursor();
        if (idx == -1) {
            return;
        }
        if (idx == 0) {
            LOG_WARN("Mapping guard page not allowed");
            return;
        }
        if (focus.is_selected && is_proc_same(focus.proc, proc) &&
            focus.page_table_idx == (size_t)idx) {
            focus.is_selected = false;
        } else {
            focus.page_table_idx = idx;
            focus.proc = proc;
            focus.is_selected = true;
            viewport_offset = 0;
        }
    }
}

static void scroll_handler() {
    int scroll_multiplier = 4;
    viewport_offset += GetMouseWheelMove() * scroll_multiplier;
    if (viewport_offset < 0) {
        viewport_offset = 0;
    }
}

static void draw_memory_inspector() {
    int font_size = 14;
    int start_x = 980;
    int start_y = TOP_PADDING;
    int offset = viewport_offset;

    int total_rows = 48;
    int total_cols = 16;

    for (int i = 0; i < total_rows; i++) {
        int idx = (i + offset) * total_cols;
        if (focus.is_selected) {
            idx += focus.page_table_idx << 12;
        }
        DrawText(TextFormat("%03X: ", idx), start_x - 50, start_y + (i * 15), font_size,
                 BLUE);
        for (int j = 0; j < total_cols; j++) {
            unsigned char data = inspect_memory(proc, idx + j);
            int pos_x = start_x + j * 25;
            int pos_y = start_y + i * 15;
            DrawText(TextFormat("%02X", data), pos_x, pos_y, font_size, HEX_RED_COLOR);
        }
    }

    start_x += total_cols * 25;
    for (int i = 0; i < total_rows; i++) {
        int idx = (i + offset) * total_cols;
        for (int j = 0; j < total_cols; j++) {
            unsigned char data = inspect_memory(proc, idx + j);
            int pos_x = start_x + j * 10;
            int pos_y = start_y + i * 15;
            if (isprint(data)) {
                DrawText(TextFormat("%c", data), pos_x, pos_y, font_size, BLUE);
            } else {
                DrawText(TextFormat("."), pos_x, pos_y, font_size, BLUE);
            }
        }
    }
}

static void render_loop() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);

        DrawText("Virtual memory with memory inspector", 450, GetScreenWidth() / 64, 40,
                 TITLE_COLOR);

        size_t left_padding = LEFT_PADDING;
        draw_page_table(proc, left_padding);
        draw_physical_memory();
        draw_memory_inspector();

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc->page_table->entries[i] != 0) {
                size_t frame_idx = proc->page_table->entries[i] >> 12;
                draw_arrow_from_proc_left(i, frame_idx);
            }
        }

        mouse_click_handler();
        keyboard_handler();
        scroll_handler();

        EndDrawing();
    }
}
static void init_visualsation() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Paging Simulator");

    render_loop();

    CloseWindow();
}

void memory_inspector_visualisation(struct Proc *_proc) {
    proc = _proc;
    proc->page_table->size = sim_page_size;

    // create_test_case_1();

    init_visualsation();

    // free(test_case.ops);
}
