#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CANVAS_WIDTH 40
#define CANVAS_HEIGHT 20
#define MAX_OBJECTS 100
#define MAX_HISTORY 20
#define PI 3.14159265358979323846

typedef enum {
    CIRCLE,
    RECTANGLE,
    LINE,
    TRIANGLE
} ShapeType;

typedef struct {
    ShapeType type;
    int x1, y1, x2, y2, x3, y3;
    int radius;
    char symbol;
    int filled;
} Shape;

typedef struct {
    Shape snapshot[MAX_OBJECTS];
    int count;
} HistoryState;

char canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
Shape objects[MAX_OBJECTS];
int object_count = 0;
HistoryState undo_stack[MAX_HISTORY];
HistoryState redo_stack[MAX_HISTORY];
int undo_top = -1;
int redo_top = -1;

void init_canvas() {
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            canvas[y][x] = '_';
        }
    }
}

int read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    return 1;
}

int read_int(const char *prompt, int *out) {
    char buf[128];
    if (prompt) printf("%s", prompt);
    if (!read_line(buf, sizeof(buf))) return 0;
    char *endptr;
    long v = strtol(buf, &endptr, 10);
    if (endptr == buf) return 0;
    *out = (int)v;
    return 1;
}

int read_char(const char *prompt, char *out) {
    char buf[16];
    if (prompt) printf("%s", prompt);
    if (!read_line(buf, sizeof(buf))) return 0;
    if (buf[0] == '\0') return 0;
    *out = buf[0];
    return 1;
}

void draw_point(int x, int y, char symbol) {
    if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
        canvas[y][x] = symbol;
    }
}

void draw_line(int x0, int y0, int x1, int y1, char symbol) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int x = x0, y = y0;
    while (1) {
        draw_point(x, y, symbol);
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void draw_rectangle(int x1, int y1, int x2, int y2, char symbol) {
    int min_x = (x1 < x2) ? x1 : x2;
    int max_x = (x1 < x2) ? x2 : x1;
    int min_y = (y1 < y2) ? y1 : y2;
    int max_y = (y1 < y2) ? y2 : y1;
    for (int x = min_x; x <= max_x; x++) {
        draw_point(x, min_y, symbol);
        draw_point(x, max_y, symbol);
    }
    for (int y = min_y; y <= max_y; y++) {
        draw_point(min_x, y, symbol);
        draw_point(max_x, y, symbol);
    }
}

void fill_rectangle(int x1, int y1, int x2, int y2, char symbol) {
    int min_x = (x1 < x2) ? x1 : x2;
    int max_x = (x1 < x2) ? x2 : x1;
    int min_y = (y1 < y2) ? y1 : y2;
    int max_y = (y1 < y2) ? y2 : y1;
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            draw_point(x, y, symbol);
        }
    }
}

void draw_circle(int cx, int cy, int radius, char symbol) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    while (x <= y) {
        draw_point(cx + x, cy + y, symbol);
        draw_point(cx - x, cy + y, symbol);
        draw_point(cx + x, cy - y, symbol);
        draw_point(cx - x, cy - y, symbol);
        draw_point(cx + y, cy + x, symbol);
        draw_point(cx - y, cy + x, symbol);
        draw_point(cx + y, cy - x, symbol);
        draw_point(cx - y, cy - x, symbol);
        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void fill_circle(int cx, int cy, int radius, char symbol) {
    for (int y = cy - radius; y <= cy + radius; y++) {
        for (int x = cx - radius; x <= cx + radius; x++) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= radius * radius) {
                draw_point(x, y, symbol);
            }
        }
    }
}

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, char symbol) {
    draw_line(x1, y1, x2, y2, symbol);
    draw_line(x2, y2, x3, y3, symbol);
    draw_line(x3, y3, x1, y1, symbol);
}

int triangle_sign(int x1, int y1, int x2, int y2, int x3, int y3) {
    return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
}

int point_in_triangle(int px, int py, int x1, int y1, int x2, int y2, int x3, int y3) {
    int d1 = triangle_sign(px, py, x1, y1, x2, y2);
    int d2 = triangle_sign(px, py, x2, y2, x3, y3);
    int d3 = triangle_sign(px, py, x3, y3, x1, y1);
    int has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    int has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(has_neg && has_pos);
}

void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, char symbol) {
    int min_x = x1;
    int max_x = x1;
    int min_y = y1;
    int max_y = y1;
    if (x2 < min_x) min_x = x2;
    if (x3 < min_x) min_x = x3;
    if (x2 > max_x) max_x = x2;
    if (x3 > max_x) max_x = x3;
    if (y2 < min_y) min_y = y2;
    if (y3 < min_y) min_y = y3;
    if (y2 > max_y) max_y = y2;
    if (y3 > max_y) max_y = y3;
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            if (point_in_triangle(x, y, x1, y1, x2, y2, x3, y3)) {
                draw_point(x, y, symbol);
            }
        }
    }
}

void copy_state_to_snapshot(HistoryState *state) {
    state->count = object_count;
    for (int i = 0; i < object_count; i++) {
        state->snapshot[i] = objects[i];
    }
}

void restore_snapshot_to_state(const HistoryState *state) {
    object_count = state->count;
    for (int i = 0; i < object_count; i++) {
        objects[i] = state->snapshot[i];
    }
}

void clear_redo() {
    redo_top = -1;
}

void push_undo_state() {
    if (undo_top < MAX_HISTORY - 1) {
        undo_top++;
    } else {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            undo_stack[i] = undo_stack[i + 1];
        }
    }
    copy_state_to_snapshot(&undo_stack[undo_top]);
    clear_redo();
}

int can_undo() {
    return undo_top >= 0;
}

int can_redo() {
    return redo_top >= 0;
}

void push_redo_state() {
    if (redo_top < MAX_HISTORY - 1) {
        redo_top++;
    } else {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            redo_stack[i] = redo_stack[i + 1];
        }
    }
    copy_state_to_snapshot(&redo_stack[redo_top]);
}

void rebuild_canvas(void);

void undo() {
    if (!can_undo()) {
        printf("Nothing to undo.\n");
        return;
    }
    push_redo_state();
    restore_snapshot_to_state(&undo_stack[undo_top]);
    undo_top--;
    rebuild_canvas();
    printf("Undo completed.\n");
}

void redo() {
    if (!can_redo()) {
        printf("Nothing to redo.\n");
        return;
    }
    push_undo_state();
    restore_snapshot_to_state(&redo_stack[redo_top]);
    redo_top--;
    rebuild_canvas();
    printf("Redo completed.\n");
}

int add_object(ShapeType type, int x1, int y1, int x2, int y2, int x3, int y3, int radius, char symbol, int filled) {
    if (object_count >= MAX_OBJECTS) {
        printf("Error: Maximum number of objects reached!\n");
        return -1;
    }
    objects[object_count].type = type;
    objects[object_count].x1 = x1;
    objects[object_count].y1 = y1;
    objects[object_count].x2 = x2;
    objects[object_count].y2 = y2;
    objects[object_count].x3 = x3;
    objects[object_count].y3 = y3;
    objects[object_count].radius = radius;
    objects[object_count].symbol = symbol;
    objects[object_count].filled = filled;
    printf("Object %d added successfully!\n", object_count);
    return object_count++;
}

void delete_object(int index) {
    if (index < 0 || index >= object_count) {
        printf("Error: Invalid object index!\n");
        return;
    }
    for (int i = index; i < object_count - 1; i++) {
        objects[i] = objects[i + 1];
    }
    object_count--;
    printf("Object deleted successfully! Remaining objects: %d\n", object_count);
}

void move_object(int index, int dx, int dy) {
    if (index < 0 || index >= object_count) {
        printf("Error: Invalid object index!\n");
        return;
    }
    Shape *shape = &objects[index];
    shape->x1 += dx;
    shape->y1 += dy;
    shape->x2 += dx;
    shape->y2 += dy;
    shape->x3 += dx;
    shape->y3 += dy;
    printf("Object %d moved by (%d, %d).\n", index, dx, dy);
}

void duplicate_object(int index) {
    if (index < 0 || index >= object_count) {
        printf("Error: Invalid object index!\n");
        return;
    }
    if (object_count >= MAX_OBJECTS) {
        printf("Error: Maximum number of objects reached!\n");
        return;
    }
    objects[object_count] = objects[index];
    printf("Object %d duplicated at index %d.\n", index, object_count);
    object_count++;
}

void modify_object(int index) {
    if (index < 0 || index >= object_count) {
        printf("Error: Invalid object index!\n");
        return;
    }
    Shape *shape = &objects[index];
    printf("Modifying object %d:\n", index);
    printf("Current symbol: %c\n", shape->symbol);
    char tmp_ch;
    if (!read_char("Enter new symbol (or same symbol to keep): ", &tmp_ch)) { printf("Invalid input.\n"); return; }
    shape->symbol = tmp_ch;
    if (shape->type == CIRCLE || shape->type == RECTANGLE || shape->type == TRIANGLE) {
        char fillAns = 'n';
        if (read_char("Fill shape interior? (y/n): ", &fillAns) && (fillAns == 'y' || fillAns == 'Y')) {
            shape->filled = 1;
        } else {
            shape->filled = 0;
        }
    }
    switch (shape->type) {
        case CIRCLE:
            if (!read_int("Enter new center X: ", &shape->x1) || !read_int("Enter new center Y: ", &shape->y1) || !read_int("Enter new radius: ", &shape->radius)) { printf("Invalid input.\n"); return; }
            break;
        case RECTANGLE:
            if (!read_int("Enter top-left X: ", &shape->x1) || !read_int("Enter top-left Y: ", &shape->y1) || !read_int("Enter bottom-right X: ", &shape->x2) || !read_int("Enter bottom-right Y: ", &shape->y2)) { printf("Invalid input.\n"); return; }
            break;
        case LINE:
            if (!read_int("Enter start X: ", &shape->x1) || !read_int("Enter start Y: ", &shape->y1) || !read_int("Enter end X: ", &shape->x2) || !read_int("Enter end Y: ", &shape->y2)) { printf("Invalid input.\n"); return; }
            break;
        case TRIANGLE:
            if (!read_int("Enter x1: ", &shape->x1) || !read_int("Enter y1: ", &shape->y1) || !read_int("Enter x2: ", &shape->x2) || !read_int("Enter y2: ", &shape->y2) || !read_int("Enter x3: ", &shape->x3) || !read_int("Enter y3: ", &shape->y3)) { printf("Invalid input.\n"); return; }
            break;
    }
    printf("Object %d modified successfully!\n", index);
}

void rebuild_canvas() {
    init_canvas();
    for (int i = 0; i < object_count; i++) {
        Shape *shape = &objects[i];
        switch (shape->type) {
            case CIRCLE:
                if (shape->filled) fill_circle(shape->x1, shape->y1, shape->radius, shape->symbol);
                else draw_circle(shape->x1, shape->y1, shape->radius, shape->symbol);
                break;
            case RECTANGLE:
                if (shape->filled) fill_rectangle(shape->x1, shape->y1, shape->x2, shape->y2, shape->symbol);
                else draw_rectangle(shape->x1, shape->y1, shape->x2, shape->y2, shape->symbol);
                break;
            case LINE:
                draw_line(shape->x1, shape->y1, shape->x2, shape->y2, shape->symbol);
                break;
            case TRIANGLE:
                if (shape->filled) fill_triangle(shape->x1, shape->y1, shape->x2, shape->y2, shape->x3, shape->y3, shape->symbol);
                else draw_triangle(shape->x1, shape->y1, shape->x2, shape->y2, shape->x3, shape->y3, shape->symbol);
                break;
        }
    }
}

void display_canvas() {
    printf("\n");
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            printf("%c", canvas[y][x]);
        }
        printf("\n");
    }
    printf("\n");
}

void pick_point() {
    int y, x;
    printf("\n=== Pick/Set Point ===\n");
    if (!read_int("Enter row (0..19): ", &y)) { printf("Invalid input.\n"); return; }
    if (y < 0 || y >= CANVAS_HEIGHT) { printf("Row out of range.\n"); return; }
    printf("Index: ");
    for (int i = 0; i < CANVAS_WIDTH; i++) printf("%d", i % 10);
    printf("\nRow %d:  ", y);
    for (int i = 0; i < CANVAS_WIDTH; i++) putchar(canvas[y][i]);
    printf("\n");
    if (!read_int("Enter column (0..39): ", &x)) { printf("Invalid input.\n"); return; }
    if (x < 0 || x >= CANVAS_WIDTH) { printf("Column out of range.\n"); return; }
    printf("Character at (%d,%d) = '%c'\n", x, y, canvas[y][x]);
    char ans = 'n';
    if (!read_char("Do you want to change it? (y/n): ", &ans)) { printf("Invalid input.\n"); return; }
    if (ans == 'y' || ans == 'Y') {
        char symbol;
        if (!read_char("Enter new symbol: ", &symbol)) { printf("Invalid input.\n"); return; }
        push_undo_state();
        add_object(LINE, x, y, x, y, 0, 0, 0, symbol, 0);
        rebuild_canvas();
        clear_redo();
        printf("Point set at (%d,%d) with symbol '%c'.\n", x, y, symbol);
    } else {
        printf("No change made.\n");
    }
}

int select_canvas_position(const char *label, int *out_x, int *out_y) {
    int y, x;
    printf("\n=== Select %s Position ===\n", label);
    if (!read_int("Enter row (0..19): ", &y)) { printf("Invalid input.\n"); return 0; }
    if (y < 0 || y >= CANVAS_HEIGHT) { printf("Row out of range.\n"); return 0; }
    printf("Index: ");
    for (int i = 0; i < CANVAS_WIDTH; i++) printf("%d", i % 10);
    printf("\nRow %d:  ", y);
    for (int i = 0; i < CANVAS_WIDTH; i++) putchar(canvas[y][i]);
    printf("\n");
    if (!read_int("Enter column (0..39): ", &x)) { printf("Invalid input.\n"); return 0; }
    if (x < 0 || x >= CANVAS_WIDTH) { printf("Column out of range.\n"); return 0; }
    *out_x = x;
    *out_y = y;
    return 1;
}

void list_objects() {
    if (object_count == 0) {
        printf("No objects in the picture.\n");
        return;
    }
    printf("\n=== Objects in Picture ===\n");
    for (int i = 0; i < object_count; i++) {
        printf("%d. ", i);
        switch (objects[i].type) {
            case CIRCLE:
                printf("Circle at (%d, %d) radius=%d symbol='%c' %s\n",
                       objects[i].x1, objects[i].y1, objects[i].radius, objects[i].symbol,
                       objects[i].filled ? "filled" : "outline");
                break;
            case RECTANGLE:
                printf("Rectangle from (%d, %d) to (%d, %d) symbol='%c' %s\n",
                       objects[i].x1, objects[i].y1, objects[i].x2, objects[i].y2, objects[i].symbol,
                       objects[i].filled ? "filled" : "outline");
                break;
            case LINE:
                printf("Line from (%d, %d) to (%d, %d) symbol='%c'\n",
                       objects[i].x1, objects[i].y1, objects[i].x2, objects[i].y2, objects[i].symbol);
                break;
            case TRIANGLE:
                printf("Triangle (%d, %d) (%d, %d) (%d, %d) symbol='%c' %s\n",
                       objects[i].x1, objects[i].y1, objects[i].x2, objects[i].y2,
                       objects[i].x3, objects[i].y3, objects[i].symbol,
                       objects[i].filled ? "filled" : "outline");
                break;
        }
    }
    printf("\n");
}

void save_drawing(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: Could not open file '%s' for writing.\n", filename);
        return;
    }
    fprintf(file, "%d\n", object_count);
    for (int i = 0; i < object_count; i++) {
        Shape *shape = &objects[i];
        fprintf(file, "%d %d %d %d %d %d %d %d %c\n",
                shape->type, shape->x1, shape->y1, shape->x2, shape->y2,
                shape->x3, shape->y3, shape->radius, shape->symbol);
        fprintf(file, "%d\n", shape->filled);
    }
    fclose(file);
    printf("Drawing saved to '%s'.\n", filename);
}

void load_drawing(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file '%s'.\n", filename);
        return;
    }
    int count;
    if (fscanf(file, "%d", &count) != 1 || count < 0 || count > MAX_OBJECTS) {
        printf("Error: Invalid file format.\n");
        fclose(file);
        return;
    }
    Shape loaded[MAX_OBJECTS];
    for (int i = 0; i < count; i++) {
        int type, x1, y1, x2, y2, x3, y3, radius, filled;
        char symbol;
        if (fscanf(file, "%d %d %d %d %d %d %d %d %c", &type, &x1, &y1, &x2, &y2, &x3, &y3, &radius, &symbol) != 9) {
            printf("Error: Invalid object data in file.\n");
            fclose(file);
            return;
        }
        if (fscanf(file, "%d", &filled) != 1) {
            printf("Error: Invalid fill state in file.\n");
            fclose(file);
            return;
        }
        if (type < 0 || type > TRIANGLE) {
            printf("Error: Invalid shape type in file.\n");
            fclose(file);
            return;
        }
        loaded[i].type = (ShapeType)type;
        loaded[i].x1 = x1;
        loaded[i].y1 = y1;
        loaded[i].x2 = x2;
        loaded[i].y2 = y2;
        loaded[i].x3 = x3;
        loaded[i].y3 = y3;
        loaded[i].radius = radius;
        loaded[i].symbol = symbol;
        loaded[i].filled = filled ? 1 : 0;
    }
    fclose(file);
    push_undo_state();
    for (int i = 0; i < count; i++) {
        objects[i] = loaded[i];
    }
    object_count = count;
    rebuild_canvas();
    clear_redo();
    printf("Drawing loaded from '%s'.\n", filename);
}

void export_ascii_art(const char *filename) {
    rebuild_canvas();
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: Could not open file '%s' for writing.\n", filename);
        return;
    }
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            fputc(canvas[y][x], file);
        }
        fputc('\n', file);
    }
    fclose(file);
    printf("ASCII canvas exported to '%s'.\n", filename);
}

double shape_area(const Shape *shape) {
    switch (shape->type) {
        case CIRCLE:
            return PI * shape->radius * shape->radius;
        case RECTANGLE: {
            int width = abs(shape->x2 - shape->x1);
            int height = abs(shape->y2 - shape->y1);
            return width * height;
        }
        case LINE:
            return 0.0;
        case TRIANGLE:
            return fabs((shape->x1 * (shape->y2 - shape->y3) + shape->x2 * (shape->y3 - shape->y1) + shape->x3 * (shape->y1 - shape->y2)) / 2.0);
    }
    return 0.0;
}

double shape_perimeter(const Shape *shape) {
    switch (shape->type) {
        case CIRCLE:
            return 2.0 * PI * shape->radius;
        case RECTANGLE: {
            int width = abs(shape->x2 - shape->x1);
            int height = abs(shape->y2 - shape->y1);
            return 2.0 * (width + height);
        }
        case LINE: {
            double dx = shape->x2 - shape->x1;
            double dy = shape->y2 - shape->y1;
            return sqrt(dx * dx + dy * dy);
        }
        case TRIANGLE: {
            double dx1 = shape->x2 - shape->x1;
            double dy1 = shape->y2 - shape->y1;
            double dx2 = shape->x3 - shape->x2;
            double dy2 = shape->y3 - shape->y2;
            double dx3 = shape->x1 - shape->x3;
            double dy3 = shape->y1 - shape->y3;
            return sqrt(dx1*dx1 + dy1*dy1) + sqrt(dx2*dx2 + dy2*dy2) + sqrt(dx3*dx3 + dy3*dy3);
        }
    }
    return 0.0;
}

int shape_contains_coordinate(const Shape *shape, int x, int y) {
    switch (shape->type) {
        case CIRCLE: {
            int dx = x - shape->x1;
            int dy = y - shape->y1;
            return dx*dx + dy*dy <= shape->radius * shape->radius;
        }
        case RECTANGLE: {
            int min_x = (shape->x1 < shape->x2) ? shape->x1 : shape->x2;
            int max_x = (shape->x1 < shape->x2) ? shape->x2 : shape->x1;
            int min_y = (shape->y1 < shape->y2) ? shape->y1 : shape->y2;
            int max_y = (shape->y1 < shape->y2) ? shape->y2 : shape->y1;
            return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
        }
        case LINE: {
            int dx = shape->x2 - shape->x1;
            int dy = shape->y2 - shape->y1;
            int dx1 = x - shape->x1;
            int dy1 = y - shape->y1;
            int cross = dx * dy1 - dy * dx1;
            if (cross != 0) return 0;
            if (dx != 0) {
                int min_x = (shape->x1 < shape->x2) ? shape->x1 : shape->x2;
                int max_x = (shape->x1 < shape->x2) ? shape->x2 : shape->x1;
                return x >= min_x && x <= max_x;
            }
            if (dy != 0) {
                int min_y = (shape->y1 < shape->y2) ? shape->y1 : shape->y2;
                int max_y = (shape->y1 < shape->y2) ? shape->y2 : shape->y1;
                return y >= min_y && y <= max_y;
            }
            return x == shape->x1 && y == shape->y1;
        }
        case TRIANGLE:
            return point_in_triangle(x, y, shape->x1, shape->y1, shape->x2, shape->y2, shape->x3, shape->y3);
    }
    return 0;
}

void display_canvas_statistics() {
    int circles = 0, rectangles = 0, lines = 0, triangles = 0;
    for (int i = 0; i < object_count; i++) {
        switch (objects[i].type) {
            case CIRCLE: circles++; break;
            case RECTANGLE: rectangles++; break;
            case LINE: lines++; break;
            case TRIANGLE: triangles++; break;
        }
    }
    printf("\n=== Canvas Statistics ===\n");
    printf("Total objects: %d\n", object_count);
    printf("Circles: %d\n", circles);
    printf("Rectangles: %d\n", rectangles);
    printf("Lines: %d\n", lines);
    printf("Triangles: %d\n", triangles);
}

void search_objects() {
    if (object_count == 0) {
        printf("No objects to search.\n");
        return;
    }
    printf("\n=== Search Objects ===\n");
    printf("1. By type\n");
    printf("2. By symbol\n");
    printf("3. By coordinate\n");
    printf("Select (1-3): ");
    int choice;
    if (!read_int(NULL, &choice)) { printf("Invalid input.\n"); return; }
    int found = 0;
    if (choice == 1) {
        printf("Types: 1.Circle 2.Rectangle 3.Line 4.Triangle\n");
        if (!read_int("Select type: ", &choice)) { printf("Invalid input.\n"); return; }
        if (choice < 1 || choice > 4) { printf("Invalid type.\n"); return; }
        ShapeType type = (ShapeType)(choice - 1);
        for (int i = 0; i < object_count; i++) {
            if (objects[i].type == type) {
                printf("Found object %d.\n", i);
                found++;
            }
        }
    } else if (choice == 2) {
        char symbol;
        if (!read_char("Enter symbol to search: ", &symbol)) { printf("Invalid input.\n"); return; }
        for (int i = 0; i < object_count; i++) {
            if (objects[i].symbol == symbol) {
                printf("Found object %d.\n", i);
                found++;
            }
        }
    } else if (choice == 3) {
        int x, y;
        if (!read_int("Enter X coordinate: ", &x) || !read_int("Enter Y coordinate: ", &y)) { printf("Invalid input.\n"); return; }
        for (int i = 0; i < object_count; i++) {
            if (shape_contains_coordinate(&objects[i], x, y)) {
                printf("Found object %d.\n", i);
                found++;
            }
        }
    } else {
        printf("Invalid choice.\n");
        return;
    }
    if (!found) {
        printf("No matching objects found.\n");
    }
}

void show_shape_measurements() {
    if (object_count == 0) {
        printf("No objects available.\n");
        return;
    }
    list_objects();
    int index;
    if (!read_int("Enter object index to analyze: ", &index)) { printf("Invalid input.\n"); return; }
    if (index < 0 || index >= object_count) { printf("Invalid index.\n"); return; }
    double area = shape_area(&objects[index]);
    double perimeter = shape_perimeter(&objects[index]);
    printf("Object %d area: %.2f\n", index, area);
    printf("Object %d perimeter: %.2f\n", index, perimeter);
}

int validate_filename(const char *name) {
    if (name == NULL || name[0] == '\0') return 0;
    if (strchr(name, '/') || strchr(name, '\\') || strchr(name, ':')) return 0;
    return 1;
}

void menu_add_object() {
    printf("\n=== Add Object ===\n");
    printf("1. Circle\n");
    printf("2. Rectangle\n");
    printf("3. Line\n");
    printf("4. Triangle\n");
    printf("Select (1-4): ");
    int choice;
    if (!read_int(NULL, &choice)) { printf("Invalid input.\n"); return; }
    char symbol;
    if (!read_char("Enter symbol (e.g., *, _, #): ", &symbol)) { printf("Invalid input.\n"); return; }
    int x1, y1, x2, y2, x3, y3, radius;
    int filled = 0;
    if (choice == 1 || choice == 2 || choice == 4) {
        char ans = 'n';
        if (read_char("Fill shape interior? (y/n): ", &ans) && (ans == 'y' || ans == 'Y')) {
            filled = 1;
        }
    }
    switch (choice) {
        case 1: {
            char ans = 'n';
            if (read_char("Select center from canvas row? (y/n): ", &ans) && (ans == 'y' || ans == 'Y')) {
                if (!select_canvas_position("circle center", &x1, &y1)) return;
            } else {
                if (!read_int("Enter center X: ", &x1) || !read_int("Enter center Y: ", &y1)) { printf("Invalid input.\n"); return; }
            }
            if (!read_int("Enter radius: ", &radius) || radius < 0) { printf("Invalid input.\n"); return; }
            push_undo_state();
            add_object(CIRCLE, x1, y1, 0, 0, 0, 0, radius, symbol, filled);
            rebuild_canvas();
            clear_redo();
            break;
        }
        case 2: {
            char ans = 'n';
            if (read_char("Select top-left from canvas row? (y/n): ", &ans) && (ans == 'y' || ans == 'Y')) {
                if (!select_canvas_position("rectangle top-left", &x1, &y1)) return;
            } else {
                if (!read_int("Enter top-left X: ", &x1) || !read_int("Enter top-left Y: ", &y1)) { printf("Invalid input.\n"); return; }
            }
            if (!read_int("Enter bottom-right X: ", &x2) || !read_int("Enter bottom-right Y: ", &y2)) { printf("Invalid input.\n"); return; }
            push_undo_state();
            add_object(RECTANGLE, x1, y1, x2, y2, 0, 0, 0, symbol, filled);
            rebuild_canvas();
            clear_redo();
            break;
        }
        case 3: {
            char ans = 'n';
            if (read_char("Select start from canvas row? (y/n): ", &ans) && (ans == 'y' || ans == 'Y')) {
                if (!select_canvas_position("line start", &x1, &y1)) return;
            } else {
                if (!read_int("Enter start X: ", &x1) || !read_int("Enter start Y: ", &y1)) { printf("Invalid input.\n"); return; }
            }
            if (!read_int("Enter end X: ", &x2) || !read_int("Enter end Y: ", &y2)) { printf("Invalid input.\n"); return; }
            push_undo_state();
            add_object(LINE, x1, y1, x2, y2, 0, 0, 0, symbol, 0);
            rebuild_canvas();
            clear_redo();
            break;
        }
        case 4: {
            char ans = 'n';
            if (read_char("Select first vertex from canvas row? (y/n): ", &ans) && (ans == 'y' || ans == 'Y')) {
                if (!select_canvas_position("triangle vertex 1", &x1, &y1)) return;
            } else {
                if (!read_int("Enter x1: ", &x1) || !read_int("Enter y1: ", &y1)) { printf("Invalid input.\n"); return; }
            }
            if (!read_int("Enter x2: ", &x2) || !read_int("Enter y2: ", &y2) || !read_int("Enter x3: ", &x3) || !read_int("Enter y3: ", &y3)) { printf("Invalid input.\n"); return; }
            push_undo_state();
            add_object(TRIANGLE, x1, y1, x2, y2, x3, y3, 0, symbol, filled);
            rebuild_canvas();
            clear_redo();
            break;
        }
        default:
            printf("Invalid choice!\n");
    }
}

void main_menu() {
    int choice;
    while (1) {
        printf("\n===== 2D Graphics Editor =====\n");
        printf("1. Add Object\n");
        printf("2. Delete Object\n");
        printf("3. View Picture\n");
        printf("4. List Objects\n");
        printf("5. Clear All\n");
        printf("6. Pick/Set Point\n");
        printf("7. Modify Object\n");
        printf("8. Move Object\n");
        printf("9. Duplicate Object\n");
        printf("10. Search Objects\n");
        printf("11. Canvas Statistics\n");
        printf("12. Save Drawing\n");
        printf("13. Load Drawing\n");
        printf("14. Export ASCII Art\n");
        printf("15. Undo\n");
        printf("16. Redo\n");
        printf("17. Calculate Area/Perimeter\n");
        printf("18. Exit\n");
        printf("Select (1-18): ");
        if (!read_int(NULL, &choice)) { printf("Invalid input.\n"); continue; }
        switch (choice) {
            case 1:
                menu_add_object();
                display_canvas();
                break;
            case 2: {
                list_objects();
                if (object_count > 0) {
                    int index;
                    if (!read_int("Enter object index to delete: ", &index)) { printf("Invalid input.\n"); break; }
                    if (index < 0 || index >= object_count) { printf("Invalid index.\n"); break; }
                    push_undo_state();
                    delete_object(index);
                    rebuild_canvas();
                    clear_redo();
                    display_canvas();
                }
                break;
            }
            case 3:
                rebuild_canvas();
                display_canvas();
                break;
            case 4:
                list_objects();
                break;
            case 5:
                if (object_count > 0) {
                    push_undo_state();
                    object_count = 0;
                    init_canvas();
                    clear_redo();
                }
                printf("All objects cleared!\n");
                break;
            case 6:
                pick_point();
                display_canvas();
                break;
            case 7: {
                list_objects();
                if (object_count > 0) {
                    int index;
                    if (!read_int("Enter object index to modify: ", &index)) { printf("Invalid input.\n"); break; }
                    if (index < 0 || index >= object_count) { printf("Invalid index.\n"); break; }
                    push_undo_state();
                    modify_object(index);
                    rebuild_canvas();
                    clear_redo();
                    display_canvas();
                }
                break;
            }
            case 8: {
                list_objects();
                if (object_count > 0) {
                    int index, dx, dy;
                    if (!read_int("Enter object index to move: ", &index) || !read_int("Enter X offset: ", &dx) || !read_int("Enter Y offset: ", &dy)) { printf("Invalid input.\n"); break; }
                    if (index < 0 || index >= object_count) { printf("Invalid index.\n"); break; }
                    push_undo_state();
                    move_object(index, dx, dy);
                    rebuild_canvas();
                    clear_redo();
                    display_canvas();
                }
                break;
            }
            case 9: {
                list_objects();
                if (object_count > 0) {
                    int index;
                    if (!read_int("Enter object index to duplicate: ", &index)) { printf("Invalid input.\n"); break; }
                    if (index < 0 || index >= object_count) { printf("Invalid index.\n"); break; }
                    push_undo_state();
                    duplicate_object(index);
                    rebuild_canvas();
                    clear_redo();
                    display_canvas();
                }
                break;
            }
            case 10:
                search_objects();
                break;
            case 11:
                display_canvas_statistics();
                break;
            case 12: {
                char filename[128];
                printf("Enter file name to save: ");
                if (!read_line(filename, sizeof(filename)) || !validate_filename(filename)) { printf("Invalid file name.\n"); break; }
                save_drawing(filename);
                break;
            }
            case 13: {
                char filename[128];
                printf("Enter file name to load: ");
                if (!read_line(filename, sizeof(filename)) || !validate_filename(filename)) { printf("Invalid file name.\n"); break; }
                load_drawing(filename);
                break;
            }
            case 14: {
                char filename[128];
                printf("Enter file name to export ASCII art: ");
                if (!read_line(filename, sizeof(filename)) || !validate_filename(filename)) { printf("Invalid file name.\n"); break; }
                export_ascii_art(filename);
                break;
            }
            case 15:
                undo();
                display_canvas();
                break;
            case 16:
                redo();
                display_canvas();
                break;
            case 17:
                show_shape_measurements();
                break;
            case 18:
                printf("Thank you for using 2D Graphics Editor!\n");
                return;
            default:
                printf("Invalid choice!\n");
        }
    }
}

int main() {
    init_canvas();
    main_menu();
    return 0;
}

