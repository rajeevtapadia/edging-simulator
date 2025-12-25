#include <math.h>
#include <raylib.h>
#include <simulator-ui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct FocusCtx focus = {0};
struct TestCase test_case = {0};

size_t sim_page_size = 10;
size_t sim_frame_count = 10;

int page_table_idx_at_cursor_left() {
    float x = GetMouseX();
    float y = GetMouseY();

    // checks if cursor is outside the page table
    if (x < LEFT_PADDING || x > LEFT_PADDING + BOX_WIDTH) {
        return -1;
    }
    if (y < TOP_PADDING || y > TOP_PADDING + BOX_HEIGHT * sim_page_size) {
        return -1;
    }

    return (y - TOP_PADDING) / BOX_HEIGHT;
}

// get page table index the cursor is pointing to for right process
int page_table_idx_at_cursor_right() {
    float x = GetMouseX();
    float y = GetMouseY();
    size_t left_padding = GetScreenWidth() - LEFT_PADDING - BOX_WIDTH;

    // checks if cursor is outside the page table
    if (x < left_padding || x > left_padding + BOX_WIDTH) {
        return -1;
    }
    if (y < TOP_PADDING || y > TOP_PADDING + BOX_HEIGHT * sim_page_size) {
        return -1;
    }

    return (y - TOP_PADDING) / BOX_HEIGHT;
}

int page_table_idx_at_cursor() {
    if (GetMouseX() < GetScreenWidth() / 2.f) {
        return page_table_idx_at_cursor_left();
    } else {
        return page_table_idx_at_cursor_right();
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

void draw_arrow_from_proc_left(size_t page_idx, size_t frame_idx) {
    Vector2 arrow_start, arrow_end;
    arrow_start.x = LEFT_PADDING + BOX_WIDTH;
    arrow_start.y = TOP_PADDING + (page_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    arrow_end.x = GetScreenWidth() / 2.f - BOX_WIDTH / 2.f;
    arrow_end.y = TOP_PADDING + (frame_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    DrawLineEx(arrow_start, arrow_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
    draw_arrow_head(arrow_start, arrow_end);
}

void draw_arrow_from_proc_right(size_t page_idx, size_t frame_idx) {
    Vector2 arrow_start, arrow_end;
    arrow_start.x = GetScreenWidth() - RIGHT_PADDING - BOX_WIDTH;
    arrow_start.y = TOP_PADDING + (page_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    arrow_end.x = GetScreenWidth() / 2.f + BOX_WIDTH / 2.f;
    arrow_end.y = TOP_PADDING + (frame_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    DrawLineEx(arrow_start, arrow_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
    draw_arrow_head(arrow_start, arrow_end);
}

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
    if (focus.is_selected && is_proc_same(focus.proc, proc)) {
        Rectangle rec = {.x = offset_x,
                         .y = focus.page_table_idx * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT + NORMAL_LINE_THICKNESS,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, NORMAL_LINE_THICKNESS, GREEN);
    }
}

void draw_physical_memory() {
    int font_size = 20;
    int offset_x = GetScreenWidth() / 2 - BOX_WIDTH / 2;
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
        unmap_page_by_virtual_addr(op->proc->page_table, op->virt_addr);
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

void draw_divider() {
    Vector2 divider_start = (Vector2){0, DIVIDER_POS};
    Vector2 divider_end = (Vector2){GetScreenWidth(), DIVIDER_POS};
    DrawLineEx(divider_start, divider_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
}
