#include <math.h>
#include <paging.h>
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 9

#define BOX_WIDTH 130
#define BOX_HEIGHT 60

#define TOP_PADDING 80
#define LEFT_PADDING 150
#define RIGHT_PADDING 150

#define NORMAL_LINE_THICKNESS 2

#define DIVIDER_POS 700

const int factor = 100;
int width = SCREEN_WIDTH * factor;
int height = SCREEN_HEIGHT * factor;

size_t sim_page_size = 10;
size_t sim_frame_count = 10;

Color BG_COLOR = (Color){20, 20, 21, 255};
Color TEXT_COLOR = (Color){174, 174, 209, 255};
Color TITLE_COLOR = (Color){187, 157, 189, 255};
Color BOX_BOUNDRY_COLOR = (Color){135, 135, 135, 255};

static struct Proc *proc1 = NULL;
static struct Proc *proc2 = NULL;

enum Action { READ, WRITE, UNMAP };

struct Operation {
    enum Action action;
    struct Proc *proc;
    unsigned char data;
    virt_addr_t virt_addr;
};

struct TestCase {
    struct Operation *ops;
    size_t operation_count;
    size_t curr_operation_idx;
};

struct GuiState {
    size_t page_table_idx;
    struct Proc *selected_proc;
    bool is_selected;
};

static struct TestCase test_case;
static struct GuiState gui_state;

void draw_page_table(struct Proc *proc, size_t offset_x) {
    int font_size = 20;
    int offset_y = TOP_PADDING;
    for (size_t i = 0; i < sim_page_size; i++) {
        Rectangle rec = {.x = offset_x,
                         .y = i * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT + NORMAL_LINE_THICKNESS,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);

        char buf[40];
        sprintf(buf, "%zu: %p", i, (void *)proc->page_table->entries[i]);
        DrawText(buf, offset_x + 10,
                 i * BOX_HEIGHT + BOX_HEIGHT / 2 - font_size / 2 + offset_y, font_size,
                 TEXT_COLOR);
    }

    // draw different color box for selected cell
    if (gui_state.is_selected && strcmp(gui_state.selected_proc->name, proc->name) == 0) {
        Rectangle rec = {.x = offset_x,
                         .y = gui_state.page_table_idx * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT + NORMAL_LINE_THICKNESS,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, NORMAL_LINE_THICKNESS, GREEN);
    }
}

void draw_physical_memory() {
    int font_size = 20;
    int offset_x = width / 2 - BOX_WIDTH / 2;
    int offset_y = TOP_PADDING;
    for (size_t i = 0; i < sim_frame_count; i++) {
        Rectangle rec = {.x = offset_x,
                         .y = i * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT + NORMAL_LINE_THICKNESS,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, 2, BOX_BOUNDRY_COLOR);

        char buf[20];
        sprintf(buf, "0x%lx", i << 12);
        DrawText(buf, offset_x + 10,
                 i * BOX_HEIGHT + BOX_HEIGHT / 2 - font_size / 2 + offset_y, font_size,
                 TEXT_COLOR);
    }
}

void draw_arrow_head(Vector2 arrow_start, Vector2 arrow_end) {
    // calculate slope of arrow body
    float dx = arrow_end.x - arrow_start.x;
    float dy = arrow_end.y - arrow_start.y;

    // calculate angle made by arrow body to positive x axis
    float angle = atan2f(dy, dx);

    float theta = .4f;
    float head_length = -50;

    /*
     * find coordinates of two points extending backwards from arrow tip
     * cos/sin: convert angle to x/y coordinates
     * head_length: extend the points backwards by head_length
     * add arrow_end.x/y: shift the points relative to arrow tip
     */
    float x1 = arrow_end.x + (head_length * cosf(angle + theta));
    float y1 = arrow_end.y + (head_length * sinf(angle + theta));

    float x2 = arrow_end.x + (head_length * cosf(angle - theta));
    float y2 = arrow_end.y + (head_length * sinf(angle - theta));

    DrawTriangle(arrow_end, (Vector2){x1, y1}, (Vector2){x2, y2}, BOX_BOUNDRY_COLOR);
}

void draw_arrow_from_proc1(size_t page_idx, size_t frame_idx) {
    Vector2 arrow_start, arrow_end;
    arrow_start.x = LEFT_PADDING + BOX_WIDTH;
    arrow_start.y = TOP_PADDING + (page_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    arrow_end.x = width / 2.f - BOX_WIDTH / 2.f;
    arrow_end.y = TOP_PADDING + (frame_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    DrawLineEx(arrow_start, arrow_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
    draw_arrow_head(arrow_start, arrow_end);
}

void draw_arrow_from_proc2(size_t page_idx, size_t frame_idx) {
    Vector2 arrow_start, arrow_end;
    arrow_start.x = width - RIGHT_PADDING - BOX_WIDTH;
    arrow_start.y = TOP_PADDING + (page_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    arrow_end.x = width / 2.f + BOX_WIDTH / 2.f;
    arrow_end.y = TOP_PADDING + (frame_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    DrawLineEx(arrow_start, arrow_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
    draw_arrow_head(arrow_start, arrow_end);
}

char *action_to_str(enum Action action) {
    switch (action) {
    case WRITE:
        return "WRITE";
    case READ:
        return "READ";
    case UNMAP:
        return "UNMAP";

    default:
        assert("Invalid action");
    }
    return NULL;
}

void operation_to_str(struct Operation *op, size_t idx, char *buf, size_t size) {
    struct Proc *proc = op->proc;
    char action[10];
    strcpy(action, action_to_str(op->action));

    // format: "3 - proc 2: WRITE 0xFF to 0x1000"
    if (op->action == WRITE) {
        snprintf(buf, size, "%zu - %s: %s 0x%X to %p", idx, proc->name, action, op->data,
                 (void *)op->virt_addr);
    } else {
        snprintf(buf, size, "%zu - %s: %s %p", idx, proc->name, action,
                 (void *)op->virt_addr);
    }
}

void draw_text_section() {
    DrawText(".text", 30, DIVIDER_POS, 20, TITLE_COLOR);
    int range_start = test_case.curr_operation_idx - 2;
    int range_end = range_start + 6;

    for (int i = range_start; i <= range_end; i++) {
        if (i < 0 || i >= (int)test_case.operation_count) {
            continue;
        }
        int x_offset = 30;
        int y_offset = DIVIDER_POS + abs(range_start - i - 1) * 30;

        if (i == (int)test_case.curr_operation_idx) {
            DrawText("> ", 15, y_offset, 20, TITLE_COLOR);
        }
        char buf[60];
        operation_to_str(&test_case.ops[i], i, buf, 60);
        DrawText(buf, x_offset, y_offset, 20, TEXT_COLOR);
    }
}

void perform_operation(struct Operation *op) {
    switch (op->action) {
    case WRITE:
        set_memory(op->proc, op->virt_addr, op->data);
        break;
    case READ:
        access_memory(op->proc, op->virt_addr);
        break;
    case UNMAP:
        break;

    default:
        assert("Invalid action");
    }
}

void print_operation(struct Operation *op) {
    switch (op->action) {
    case WRITE:
        printf(">> Action: WRITE, ");
        printf("data: %c, at %p, ", op->data, (void *)op->virt_addr);
        break;
    case READ:
        printf(">> Action: READ, ");
        printf("at %p, ", (void *)op->virt_addr);
        break;
    case UNMAP:
        printf(">> Action: UNMAP, ");
        printf("addr: %p, ", (void *)op->virt_addr);
        break;

    default:
        assert("Invalid action");
    }
    printf("by proc: %s\n", op->proc->name);
}

void next_operation() {
    if (IsKeyReleased(KEY_N) &&
        test_case.curr_operation_idx < test_case.operation_count) {
        LOG_INFO("curr_operation_idx %zu", test_case.curr_operation_idx);
        print_operation(&test_case.ops[test_case.curr_operation_idx]);
        perform_operation(&test_case.ops[test_case.curr_operation_idx]);
        test_case.curr_operation_idx++;
    }
}

void draw_divider() {
    Vector2 divider_start = (Vector2){0, DIVIDER_POS};
    Vector2 divider_end = (Vector2){GetScreenWidth(), DIVIDER_POS};
    DrawLineEx(divider_start, divider_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
}

size_t page_table_idx_at_cursor() {
    // TODO: implement
    return 1;
}

struct Proc *proc_at_cursor() {
    // TODO: implement
    return proc1;
}

void mouse_click_handler() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        size_t idx = page_table_idx_at_cursor();
        // TODO: compare procs with pid
        struct Proc *selected_proc = proc_at_cursor();
        if (gui_state.is_selected && gui_state.selected_proc == selected_proc &&
            gui_state.page_table_idx == idx) {
            gui_state.is_selected = false;
        } else {
            gui_state.page_table_idx = idx;
            gui_state.selected_proc = selected_proc;
            gui_state.is_selected = true;
        }
    }
}

void render_loop() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawText("Multi-Process Physical Memory Access", 380, width / 64, 40,
                 TITLE_COLOR);

        next_operation();

        size_t left_padding = LEFT_PADDING;
        size_t right_padding = width - left_padding - BOX_WIDTH;
        draw_page_table(proc1, left_padding);
        draw_page_table(proc2, right_padding);
        draw_physical_memory();

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc1->page_table->entries[i] != 0) {
                size_t frame_idx = proc1->page_table->entries[i] >> 12;
                draw_arrow_from_proc1(i, frame_idx);
            }
        }

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc2->page_table->entries[i] != 0) {
                size_t frame_idx = proc2->page_table->entries[i] >> 12;
                draw_arrow_from_proc2(i, frame_idx);
            }
        }

        draw_divider();
        draw_text_section();

        mouse_click_handler();

        EndDrawing();
    }
}

void init_visualsation() {
    InitWindow(width, height, "Paging Simulator");

    render_loop();

    CloseWindow();
}

void create_test_case_1() {
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

    // TODO: add UNMAP cases

    test_case.operation_count = i;
}

void multi_process_visualisation(struct Proc *_proc1, struct Proc *_proc2) {
    proc1 = _proc1;
    proc2 = _proc2;

    create_test_case_1();

    init_visualsation();

    free(test_case.ops);
}
