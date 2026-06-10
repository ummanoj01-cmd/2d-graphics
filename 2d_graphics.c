/*
 * graphics_editor.c
 *
 * 2D ASCII Graphics Editor
 *
 * Program overview:
 *   This program provides an interactive ASCII canvas of size 40x20.
 *   Users can create and manage shapes (circle, rectangle, line, triangle)
 *   with support for outline and filled styles. The editor supports object
 *   add/delete/modify/move/duplicate/search/list, canvas statistics, undo/redo,
 *   save/load drawings, export ASCII art, pick/set points, area and
 *   perimeter calculations, clear all objects, and display the current canvas.
 *
 * Data structures:
 *   Point       - integer canvas coordinate pair.
 *   Shape       - geometric object stored with endpoints, radius, symbol, and fill state.
 *   Drawing     - collection of shapes currently on the canvas.
 *   Canvas      - 40x20 character grid used for rendering.
 *   History     - undo/redo ring buffer holding snapshots of Drawing state.
 *
 * Function guide:
 *   Input handling    - prompt_string, prompt_integer_range, prompt_char, prompt_yes_no
 *   Shape creation    - create_shape_for_kind, build_circle, build_rectangle, build_line, build_triangle
 *   Shape rendering   - render_shape, render_line_segment, render_circle_outline, render_filled_shape
 *   Measurements      - shape_area, shape_perimeter, point_in_triangle, shape_contains_point
 *   File operations   - save_drawing_to_file, load_drawing_from_file, export_canvas_ascii
 *   History management- history_initialize, history_push, history_undo, history_redo
 *   Search operations - search_by_type, search_by_symbol, search_by_coordinate
 *   Menu system       - run_editor, execute_menu_action
 *
 * Menu operation guide:
 *   The main menu displays numbered commands. Enter a number to perform actions
 *   such as adding shapes, deleting objects, viewing the canvas, saving files,
 *   loading drawings, exporting ASCII art, undoing/redoing history, and exiting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define CANVAS_WIDTH 40
#define CANVAS_HEIGHT 20
#define MAX_OBJECTS 100
#define MAX_HISTORY 20
#define INPUT_BUFFER 256
#define SAVE_FILE_SIGNATURE "ASCII2DGRAPHICS"
#define SAVE_FILE_VERSION 1

typedef enum {
    SHAPE_CIRCLE = 1,
    SHAPE_RECTANGLE = 2,
    SHAPE_LINE = 3,
    SHAPE_TRIANGLE = 4
} ShapeKind;

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    ShapeKind kind;
    Point p1;
    Point p2;
    Point p3;
    int radius;
    char glyph;
    bool filled;
} Shape;

typedef struct {
    Shape items[MAX_OBJECTS];
    int count;
} Drawing;

typedef struct {
    char cells[CANVAS_HEIGHT][CANVAS_WIDTH];
} Canvas;

typedef struct {
    Drawing snapshot;
} HistoryEntry;

typedef struct {
    HistoryEntry entries[MAX_HISTORY + 1];
    int current_index;
    int max_index;
} History;

static Drawing activeDrawing = { .count = 0 };
static Canvas activeCanvas;
static History drawingHistory;

static bool trim_newline(char *text) {
    size_t length = strlen(text);
    if (length == 0) {
        return false;
    }
    if (text[length - 1] == '\n') {
        text[length - 1] = '\0';
        return true;
    }
    return false;
}

static bool prompt_string(const char *prompt, char *output, size_t size) {
    if (prompt) {
        printf("%s", prompt);
    }
    if (!fgets(output, (int)size, stdin)) {
        return false;
    }
    trim_newline(output);
    return output[0] != '\0';
}

static bool prompt_integer_range(const char *prompt, int *value, int min, int max) {
    char buffer[INPUT_BUFFER];
    if (!prompt_string(prompt, buffer, sizeof(buffer))) {
        return false;
    }
    char *end = NULL;
    long parsed = strtol(buffer, &end, 10);
    if (end == buffer || *end != '\0') {
        return false;
    }
    if (parsed < min || parsed > max) {
        return false;
    }
    *value = (int)parsed;
    return true;
}

static bool prompt_char(const char *prompt, char *output) {
    char buffer[INPUT_BUFFER];
    if (!prompt_string(prompt, buffer, sizeof(buffer))) {
        return false;
    }
    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        if (buffer[i] != ' ' && buffer[i] != '\t') {
            *output = buffer[i];
            return true;
        }
    }
    return false;
}

static bool prompt_yes_no(const char *prompt) {
    char answer = '\0';
    if (!prompt_char(prompt, &answer)) {
        return false;
    }
    return answer == 'y' || answer == 'Y';
}

static bool is_safe_filename(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    const char prohibited[] = "\\/:*?\"<>|";
    for (size_t i = 0; name[i] != '\0'; ++i) {
        if (name[i] < 32 || strchr(prohibited, name[i])) {
            return false;
        }
    }
    return true;
}

static const char *shape_kind_name(ShapeKind kind) {
    switch (kind) {
        case SHAPE_CIRCLE: return "Circle";
        case SHAPE_RECTANGLE: return "Rectangle";
        case SHAPE_LINE: return "Line";
        case SHAPE_TRIANGLE: return "Triangle";
        default: return "Unknown";
    }
}

static bool choose_canvas_coordinate(const char *description, Point *out_point) {
    if (prompt_yes_no("Choose a point from canvas display? (y/n): ")) {
        int row = 0;
        if (!prompt_integer_range("Enter row (0..19): ", &row, 0, CANVAS_HEIGHT - 1)) {
            printf("Row must be between 0 and %d.\n", CANVAS_HEIGHT - 1);
            return false;
        }
        int column = 0;
        if (!prompt_integer_range("Enter column (0..39): ", &column, 0, CANVAS_WIDTH - 1)) {
            printf("Column must be between 0 and %d.\n", CANVAS_WIDTH - 1);
            return false;
        }
        out_point->x = column;
        out_point->y = row;
        return true;
    }
    if (!prompt_integer_range("Enter X coordinate (0..39): ", &out_point->x, 0, CANVAS_WIDTH - 1)) {
        printf("X must be between 0 and %d.\n", CANVAS_WIDTH - 1);
        return false;
    }
    if (!prompt_integer_range("Enter Y coordinate (0..19): ", &out_point->y, 0, CANVAS_HEIGHT - 1)) {
        printf("Y must be between 0 and %d.\n", CANVAS_HEIGHT - 1);
        return false;
    }
    return true;
}

static Shape build_circle(void) {
    Shape shape;
    shape.kind = SHAPE_CIRCLE;
    shape.filled = prompt_yes_no("Fill circle interior? (y/n): ");
    prompt_char("Enter symbol for circle: ", &shape.glyph);
    choose_canvas_coordinate("circle center", &shape.p1);
    prompt_integer_range("Enter radius (0..20): ", &shape.radius, 0, 20);
    shape.p2.x = shape.p2.y = shape.p3.x = shape.p3.y = 0;
    return shape;
}

static Shape build_rectangle(void) {
    Shape shape;
    shape.kind = SHAPE_RECTANGLE;
    shape.filled = prompt_yes_no("Fill rectangle interior? (y/n): ");
    prompt_char("Enter symbol for rectangle: ", &shape.glyph);
    choose_canvas_coordinate("rectangle corner 1", &shape.p1);
    choose_canvas_coordinate("rectangle corner 2", &shape.p2);
    shape.radius = 0;
    shape.p3.x = shape.p3.y = 0;
    return shape;
}

static Shape build_line(void) {
    Shape shape;
    shape.kind = SHAPE_LINE;
    shape.filled = false;
    prompt_char("Enter symbol for line: ", &shape.glyph);
    choose_canvas_coordinate("line start", &shape.p1);
    choose_canvas_coordinate("line end", &shape.p2);
    shape.radius = 0;
    shape.p3.x = shape.p3.y = 0;
    return shape;
}

static Shape build_triangle(void) {
    Shape shape;
    shape.kind = SHAPE_TRIANGLE;
    shape.filled = prompt_yes_no("Fill triangle interior? (y/n): ");
    prompt_char("Enter symbol for triangle: ", &shape.glyph);
    choose_canvas_coordinate("triangle vertex 1", &shape.p1);
    choose_canvas_coordinate("triangle vertex 2", &shape.p2);
    choose_canvas_coordinate("triangle vertex 3", &shape.p3);
    shape.radius = 0;
    return shape;
}

static bool create_shape_for_kind(ShapeKind kind, Shape *shape) {
    if (shape == NULL) {
        return false;
    }
    switch (kind) {
        case SHAPE_CIRCLE:
            *shape = build_circle();
            return true;
        case SHAPE_RECTANGLE:
            *shape = build_rectangle();
            return true;
        case SHAPE_LINE:
            *shape = build_line();
            return true;
        case SHAPE_TRIANGLE:
            *shape = build_triangle();
            return true;
        default:
            return false;
    }
}

static double calculate_distance(Point a, Point b) {
    double dx = (double)(a.x - b.x);
    double dy = (double)(a.y - b.y);
    return sqrt(dx * dx + dy * dy);
}

static double shape_area(const Shape *shape) {
    if (shape == NULL) {
        return 0.0;
    }
    switch (shape->kind) {
        case SHAPE_CIRCLE:
            return M_PI * shape->radius * shape->radius;
        case SHAPE_RECTANGLE: {
            int width = abs(shape->p2.x - shape->p1.x);
            int height = abs(shape->p2.y - shape->p1.y);
            return (double)width * height;
        }
        case SHAPE_LINE:
            return 0.0;
        case SHAPE_TRIANGLE: {
            double dx1 = shape->p2.x - shape->p1.x;
            double dy1 = shape->p2.y - shape->p1.y;
            double dx2 = shape->p3.x - shape->p1.x;
            double dy2 = shape->p3.y - shape->p1.y;
            return fabs((dx1 * dy2 - dx2 * dy1) * 0.5);
        }
        default:
            return 0.0;
    }
}

static double shape_perimeter(const Shape *shape) {
    if (shape == NULL) {
        return 0.0;
    }
    switch (shape->kind) {
        case SHAPE_CIRCLE:
            return 2.0 * M_PI * shape->radius;
        case SHAPE_RECTANGLE: {
            int width = abs(shape->p2.x - shape->p1.x);
            int height = abs(shape->p2.y - shape->p1.y);
            return 2.0 * (width + height);
        }
        case SHAPE_LINE:
            return calculate_distance(shape->p1, shape->p2);
        case SHAPE_TRIANGLE:
            return calculate_distance(shape->p1, shape->p2)
                 + calculate_distance(shape->p2, shape->p3)
                 + calculate_distance(shape->p3, shape->p1);
        default:
            return 0.0;
    }
}

static bool point_in_triangle(Point p, Point a, Point b, Point c) {
    double denominator = ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
    if (denominator == 0.0) {
        return false;
    }
    double wa = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / denominator;
    double wb = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / denominator;
    double wc = 1.0 - wa - wb;
    return wa >= 0.0 && wb >= 0.0 && wc >= 0.0;
}

static bool shape_contains_point(const Shape *shape, Point location) {
    if (shape == NULL) {
        return false;
    }
    switch (shape->kind) {
        case SHAPE_CIRCLE: {
            int dx = location.x - shape->p1.x;
            int dy = location.y - shape->p1.y;
            return dx * dx + dy * dy <= shape->radius * shape->radius;
        }
        case SHAPE_RECTANGLE: {
            int left = shape->p1.x < shape->p2.x ? shape->p1.x : shape->p2.x;
            int right = shape->p1.x < shape->p2.x ? shape->p2.x : shape->p1.x;
            int top = shape->p1.y < shape->p2.y ? shape->p1.y : shape->p2.y;
            int bottom = shape->p1.y < shape->p2.y ? shape->p2.y : shape->p1.y;
            return location.x >= left && location.x <= right &&
                   location.y >= top && location.y <= bottom;
        }
        case SHAPE_LINE: {
            int dx = shape->p2.x - shape->p1.x;
            int dy = shape->p2.y - shape->p1.y;
            int dxp = location.x - shape->p1.x;
            int dyp = location.y - shape->p1.y;
            if (dx * dyp != dy * dxp) {
                return false;
            }
            if (abs(dx) >= abs(dy)) {
                int minx = shape->p1.x < shape->p2.x ? shape->p1.x : shape->p2.x;
                int maxx = shape->p1.x < shape->p2.x ? shape->p2.x : shape->p1.x;
                return location.x >= minx && location.x <= maxx;
            }
            int miny = shape->p1.y < shape->p2.y ? shape->p1.y : shape->p2.y;
            int maxy = shape->p1.y < shape->p2.y ? shape->p2.y : shape->p1.y;
            return location.y >= miny && location.y <= maxy;
        }
        case SHAPE_TRIANGLE:
            return point_in_triangle(location, shape->p1, shape->p2, shape->p3);
        default:
            return false;
    }
}

static void canvas_clear(Canvas *canvas) {
    for (int row = 0; row < CANVAS_HEIGHT; ++row) {
        for (int col = 0; col < CANVAS_WIDTH; ++col) {
            canvas->cells[row][col] = '_';
        }
    }
}

static void canvas_plot(Canvas *canvas, int x, int y, char glyph) {
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) {
        return;
    }
    canvas->cells[y][x] = glyph;
}

static void render_line_segment(Canvas *canvas, Point a, Point b, char glyph) {
    int dx = abs(b.x - a.x);
    int dy = abs(b.y - a.y);
    int sx = a.x < b.x ? 1 : -1;
    int sy = a.y < b.y ? 1 : -1;
    int err = dx - dy;
    int x = a.x;
    int y = a.y;

    while (true) {
        canvas_plot(canvas, x, y, glyph);
        if (x == b.x && y == b.y) {
            break;
        }
        int e2 = err * 2;
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

static void render_rectangle_outline(Canvas *canvas, Point a, Point b, char glyph) {
    Point c = { a.x, b.y };
    Point d = { b.x, a.y };
    render_line_segment(canvas, a, c, glyph);
    render_line_segment(canvas, c, b, glyph);
    render_line_segment(canvas, b, d, glyph);
    render_line_segment(canvas, d, a, glyph);
}

static void render_triangle_outline(Canvas *canvas, const Shape *shape) {
    render_line_segment(canvas, shape->p1, shape->p2, shape->glyph);
    render_line_segment(canvas, shape->p2, shape->p3, shape->glyph);
    render_line_segment(canvas, shape->p3, shape->p1, shape->glyph);
}

static void render_circle_outline(Canvas *canvas, Point center, int radius, char glyph) {
    int x = radius;
    int y = 0;
    int decision = 1 - radius;

    while (y <= x) {
        canvas_plot(canvas, center.x + x, center.y + y, glyph);
        canvas_plot(canvas, center.x - x, center.y + y, glyph);
        canvas_plot(canvas, center.x + x, center.y - y, glyph);
        canvas_plot(canvas, center.x - x, center.y - y, glyph);
        canvas_plot(canvas, center.x + y, center.y + x, glyph);
        canvas_plot(canvas, center.x - y, center.y + x, glyph);
        canvas_plot(canvas, center.x + y, center.y - x, glyph);
        canvas_plot(canvas, center.x - y, center.y - x, glyph);
        y += 1;
        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            x -= 1;
            decision += 2 * (y - x) + 1;
        }
    }
}

static void render_filled_shape(Canvas *canvas, const Shape *shape) {
    int x_min = CANVAS_WIDTH;
    int x_max = 0;
    int y_min = CANVAS_HEIGHT;
    int y_max = 0;

    if (shape->kind == SHAPE_CIRCLE) {
        x_min = shape->p1.x - shape->radius;
        x_max = shape->p1.x + shape->radius;
        y_min = shape->p1.y - shape->radius;
        y_max = shape->p1.y + shape->radius;
    } else if (shape->kind == SHAPE_RECTANGLE) {
        x_min = shape->p1.x < shape->p2.x ? shape->p1.x : shape->p2.x;
        x_max = shape->p1.x < shape->p2.x ? shape->p2.x : shape->p1.x;
        y_min = shape->p1.y < shape->p2.y ? shape->p1.y : shape->p2.y;
        y_max = shape->p1.y < shape->p2.y ? shape->p2.y : shape->p1.y;
    } else if (shape->kind == SHAPE_TRIANGLE) {
        x_min = shape->p1.x;
        x_max = shape->p1.x;
        y_min = shape->p1.y;
        y_max = shape->p1.y;
        if (shape->p2.x < x_min) x_min = shape->p2.x;
        if (shape->p3.x < x_min) x_min = shape->p3.x;
        if (shape->p2.x > x_max) x_max = shape->p2.x;
        if (shape->p3.x > x_max) x_max = shape->p3.x;
        if (shape->p2.y < y_min) y_min = shape->p2.y;
        if (shape->p3.y < y_min) y_min = shape->p3.y;
        if (shape->p2.y > y_max) y_max = shape->p2.y;
        if (shape->p3.y > y_max) y_max = shape->p3.y;
    } else {
        x_min = x_max = shape->p1.x;
        y_min = y_max = shape->p1.y;
    }

    if (x_min < 0) x_min = 0;
    if (y_min < 0) y_min = 0;
    if (x_max >= CANVAS_WIDTH) x_max = CANVAS_WIDTH - 1;
    if (y_max >= CANVAS_HEIGHT) y_max = CANVAS_HEIGHT - 1;

    for (int row = y_min; row <= y_max; ++row) {
        for (int col = x_min; col <= x_max; ++col) {
            Point test = { col, row };
            if (shape_contains_point(shape, test)) {
                canvas_plot(canvas, col, row, shape->glyph);
            }
        }
    }
}

static void render_shape(Canvas *canvas, const Shape *shape) {
    if (shape == NULL || canvas == NULL) {
        return;
    }
    if (shape->filled && shape->kind != SHAPE_LINE) {
        render_filled_shape(canvas, shape);
        return;
    }
    switch (shape->kind) {
        case SHAPE_CIRCLE:
            render_circle_outline(canvas, shape->p1, shape->radius, shape->glyph);
            break;
        case SHAPE_RECTANGLE:
            render_rectangle_outline(canvas, shape->p1, shape->p2, shape->glyph);
            break;
        case SHAPE_LINE:
            render_line_segment(canvas, shape->p1, shape->p2, shape->glyph);
            break;
        case SHAPE_TRIANGLE:
            render_triangle_outline(canvas, shape);
            break;
        default:
            break;
    }
}

static void refresh_canvas(Canvas *canvas, const Drawing *drawing) {
    canvas_clear(canvas);
    if (drawing == NULL) {
        return;
    }
    for (int index = 0; index < drawing->count; ++index) {
        render_shape(canvas, &drawing->items[index]);
    }
}

static void display_canvas(const Canvas *canvas) {
    if (canvas == NULL) {
        return;
    }
    printf("\n");
    for (int row = 0; row < CANVAS_HEIGHT; ++row) {
        for (int col = 0; col < CANVAS_WIDTH; ++col) {
            putchar(canvas->cells[row][col]);
        }
        putchar('\n');
    }
    printf("\n");
}

static bool shape_index_valid(int index) {
    return index >= 0 && index < activeDrawing.count;
}

static bool add_shape_to_drawing(const Shape *shape) {
    if (shape == NULL || activeDrawing.count >= MAX_OBJECTS) {
        return false;
    }
    activeDrawing.items[activeDrawing.count++] = *shape;
    return true;
}

static bool delete_shape_from_drawing(int index) {
    if (!shape_index_valid(index)) {
        return false;
    }
    for (int i = index; i + 1 < activeDrawing.count; ++i) {
        activeDrawing.items[i] = activeDrawing.items[i + 1];
    }
    activeDrawing.count -= 1;
    return true;
}

static bool duplicate_shape_in_drawing(int index) {
    if (!shape_index_valid(index) || activeDrawing.count >= MAX_OBJECTS) {
        return false;
    }
    activeDrawing.items[activeDrawing.count++] = activeDrawing.items[index];
    return true;
}

static bool move_shape_in_drawing(int index, int dx, int dy) {
    if (!shape_index_valid(index)) {
        return false;
    }
    Shape *shape = &activeDrawing.items[index];
    shape->p1.x += dx; shape->p1.y += dy;
    shape->p2.x += dx; shape->p2.y += dy;
    shape->p3.x += dx; shape->p3.y += dy;
    return true;
}

static void clear_drawing(void) {
    activeDrawing.count = 0;
}

static void replace_shape_fields(Shape *shape) {
    if (shape == NULL) {
        return;
    }
    prompt_char("Enter a new symbol (single character): ", &shape->glyph);
    if (shape->kind != SHAPE_LINE) {
        shape->filled = prompt_yes_no("Should the shape be filled? (y/n): ");
    } else {
        shape->filled = false;
    }
    switch (shape->kind) {
        case SHAPE_CIRCLE:
            choose_canvas_coordinate("circle center", &shape->p1);
            prompt_integer_range("Enter radius (0..20): ", &shape->radius, 0, 20);
            break;
        case SHAPE_RECTANGLE:
            choose_canvas_coordinate("rectangle corner 1", &shape->p1);
            choose_canvas_coordinate("rectangle corner 2", &shape->p2);
            break;
        case SHAPE_LINE:
            choose_canvas_coordinate("line start", &shape->p1);
            choose_canvas_coordinate("line end", &shape->p2);
            break;
        case SHAPE_TRIANGLE:
            choose_canvas_coordinate("triangle vertex 1", &shape->p1);
            choose_canvas_coordinate("triangle vertex 2", &shape->p2);
            choose_canvas_coordinate("triangle vertex 3", &shape->p3);
            break;
        default:
            break;
    }
}

static void print_shape_summary(int index, const Shape *shape) {
    if (shape == NULL) {
        return;
    }
    printf("%2d: %s ", index, shape_kind_name(shape->kind));
    switch (shape->kind) {
        case SHAPE_CIRCLE:
            printf("center=(%d,%d) radius=%d", shape->p1.x, shape->p1.y, shape->radius);
            break;
        case SHAPE_RECTANGLE:
            printf("(%d,%d)-(%d,%d)", shape->p1.x, shape->p1.y, shape->p2.x, shape->p2.y);
            break;
        case SHAPE_LINE:
            printf("(%d,%d)->(%d,%d)", shape->p1.x, shape->p1.y, shape->p2.x, shape->p2.y);
            break;
        case SHAPE_TRIANGLE:
            printf("(%d,%d),(%d,%d),(%d,%d)", shape->p1.x, shape->p1.y, shape->p2.x, shape->p2.y, shape->p3.x, shape->p3.y);
            break;
        default:
            break;
    }
    printf(" %s symbol='%c'\n", shape->filled ? "filled" : "outline", shape->glyph);
}

static void list_all_shapes(void) {
    if (activeDrawing.count == 0) {
        printf("There are no objects to list.\n");
        return;
    }
    printf("\n=== Object List ===\n");
    for (int index = 0; index < activeDrawing.count; ++index) {
        print_shape_summary(index, &activeDrawing.items[index]);
    }
    printf("\n");
}

static void show_canvas_statistics(void) {
    int counts[5] = {0};
    for (int i = 0; i < activeDrawing.count; ++i) {
        ShapeKind kind = activeDrawing.items[i].kind;
        if (kind >= SHAPE_CIRCLE && kind <= SHAPE_TRIANGLE) {
            counts[kind] += 1;
        }
    }
    printf("\n=== Canvas Statistics ===\n");
    printf("Total objects: %d\n", activeDrawing.count);
    printf("Circles: %d\n", counts[SHAPE_CIRCLE]);
    printf("Rectangles: %d\n", counts[SHAPE_RECTANGLE]);
    printf("Lines: %d\n", counts[SHAPE_LINE]);
    printf("Triangles: %d\n", counts[SHAPE_TRIANGLE]);
    printf("\n");
}

static bool save_drawing_to_file(const char *filename) {
    if (!is_safe_filename(filename)) {
        return false;
    }
    FILE *out = fopen(filename, "w");
    if (out == NULL) {
        return false;
    }
    fprintf(out, "%s\n%d\n%d\n", SAVE_FILE_SIGNATURE, SAVE_FILE_VERSION, activeDrawing.count);
    for (int i = 0; i < activeDrawing.count; ++i) {
        const Shape *s = &activeDrawing.items[i];
        fprintf(out,
                "%d %d %d %d %d %d %d %d %c %d\n",
                (int)s->kind,
                s->p1.x, s->p1.y,
                s->p2.x, s->p2.y,
                s->p3.x, s->p3.y,
                s->radius,
                s->glyph,
                s->filled ? 1 : 0);
    }
    fclose(out);
    return true;
}

static bool parse_shape_line(const char *line, Shape *shape) {
    if (line == NULL || shape == NULL) {
        return false;
    }
    int kind = 0;
    int filled = 0;
    char glyph = 0;
    int values[8] = {0};
    if (sscanf(line,
               "%d %d %d %d %d %d %d %d %c %d",
               &kind,
               &values[0], &values[1],
               &values[2], &values[3],
               &values[4], &values[5],
               &values[6],
               &glyph,
               &filled) != 10) {
        return false;
    }
    if (kind < SHAPE_CIRCLE || kind > SHAPE_TRIANGLE) {
        return false;
    }
    shape->kind = (ShapeKind)kind;
    shape->p1.x = values[0];
    shape->p1.y = values[1];
    shape->p2.x = values[2];
    shape->p2.y = values[3];
    shape->p3.x = values[4];
    shape->p3.y = values[5];
    shape->radius = values[6];
    shape->glyph = glyph;
    shape->filled = filled != 0;
    return true;
}

static bool load_drawing_from_file(const char *filename) {
    if (!is_safe_filename(filename)) {
        return false;
    }
    FILE *in = fopen(filename, "r");
    if (in == NULL) {
        return false;
    }
    char header[INPUT_BUFFER];
    if (!fgets(header, sizeof(header), in)) {
        fclose(in);
        return false;
    }
    trim_newline(header);
    if (strcmp(header, SAVE_FILE_SIGNATURE) != 0) {
        fclose(in);
        return false;
    }
    int version = 0;
    if (fscanf(in, "%d\n", &version) != 1 || version != SAVE_FILE_VERSION) {
        fclose(in);
        return false;
    }
    int count = 0;
    if (fscanf(in, "%d\n", &count) != 1 || count < 0 || count > MAX_OBJECTS) {
        fclose(in);
        return false;
    }
    Drawing imported = { .count = 0 };
    char line[INPUT_BUFFER];
    for (int i = 0; i < count; ++i) {
        if (!fgets(line, sizeof(line), in)) {
            fclose(in);
            return false;
        }
        trim_newline(line);
        if (!parse_shape_line(line, &imported.items[imported.count])) {
            fclose(in);
            return false;
        }
        imported.count += 1;
    }
    fclose(in);
    activeDrawing = imported;
    return true;
}

static bool export_canvas_ascii(const char *filename) {
    if (!is_safe_filename(filename)) {
        return false;
    }
    FILE *out = fopen(filename, "w");
    if (out == NULL) {
        return false;
    }
    for (int row = 0; row < CANVAS_HEIGHT; ++row) {
        for (int col = 0; col < CANVAS_WIDTH; ++col) {
            fputc(activeCanvas.cells[row][col], out);
        }
        fputc('\n', out);
    }
    fclose(out);
    return true;
}

static void history_initialize(void) {
    drawingHistory.current_index = 0;
    drawingHistory.max_index = 0;
    drawingHistory.entries[0].snapshot = activeDrawing;
}

static void history_push(void) {
    if (drawingHistory.current_index < drawingHistory.max_index) {
        drawingHistory.max_index = drawingHistory.current_index;
    }
    if (drawingHistory.max_index < MAX_HISTORY) {
        drawingHistory.max_index += 1;
        drawingHistory.current_index += 1;
        drawingHistory.entries[drawingHistory.current_index].snapshot = activeDrawing;
        return;
    }
    for (int i = 0; i < MAX_HISTORY; ++i) {
        drawingHistory.entries[i] = drawingHistory.entries[i + 1];
    }
    drawingHistory.entries[MAX_HISTORY].snapshot = activeDrawing;
    drawingHistory.current_index = MAX_HISTORY;
    drawingHistory.max_index = MAX_HISTORY;
}

static bool history_can_undo(void) {
    return drawingHistory.current_index > 0;
}

static bool history_can_redo(void) {
    return drawingHistory.current_index < drawingHistory.max_index;
}

static void history_undo(void) {
    if (!history_can_undo()) {
        printf("Nothing left to undo.\n");
        return;
    }
    drawingHistory.current_index -= 1;
    activeDrawing = drawingHistory.entries[drawingHistory.current_index].snapshot;
    refresh_canvas(&activeCanvas, &activeDrawing);
    printf("Undo applied.\n");
}

static void history_redo(void) {
    if (!history_can_redo()) {
        printf("Nothing left to redo.\n");
        return;
    }
    drawingHistory.current_index += 1;
    activeDrawing = drawingHistory.entries[drawingHistory.current_index].snapshot;
    refresh_canvas(&activeCanvas, &activeDrawing);
    printf("Redo applied.\n");
}

static void search_by_type(void) {
    printf("Search shapes by type:\n");
    printf("1. Circle\n2. Rectangle\n3. Line\n4. Triangle\n");
    int selection = 0;
    if (!prompt_integer_range("Select type (1-4): ", &selection, 1, 4)) {
        printf("Invalid selection.\n");
        return;
    }
    ShapeKind target = (ShapeKind)selection;
    int found = 0;
    for (int i = 0; i < activeDrawing.count; ++i) {
        if (activeDrawing.items[i].kind == target) {
            print_shape_summary(i, &activeDrawing.items[i]);
            found += 1;
        }
    }
    if (found == 0) {
        printf("No shapes of that type were found.\n");
    }
}

static void search_by_symbol(void) {
    char symbol = '\0';
    if (!prompt_char("Enter symbol to search: ", &symbol)) {
        printf("Invalid symbol.\n");
        return;
    }
    int found = 0;
    for (int i = 0; i < activeDrawing.count; ++i) {
        if (activeDrawing.items[i].glyph == symbol) {
            print_shape_summary(i, &activeDrawing.items[i]);
            found += 1;
        }
    }
    if (found == 0) {
        printf("No shapes with that symbol were found.\n");
    }
}

static void search_by_coordinate(void) {
    Point location;
    if (!prompt_integer_range("Enter search X (0..39): ", &location.x, 0, CANVAS_WIDTH - 1) ||
        !prompt_integer_range("Enter search Y (0..19): ", &location.y, 0, CANVAS_HEIGHT - 1)) {
        printf("Invalid coordinate.\n");
        return;
    }
    int found = 0;
    for (int i = 0; i < activeDrawing.count; ++i) {
        if (shape_contains_point(&activeDrawing.items[i], location)) {
            print_shape_summary(i, &activeDrawing.items[i]);
            found += 1;
        }
    }
    if (found == 0) {
        printf("No shapes intersect that point.\n");
    }
}

static void perform_object_search(void) {
    if (activeDrawing.count == 0) {
        printf("No objects available to search.\n");
        return;
    }
    printf("\nSearch options:\n");
    printf("1. By type\n");
    printf("2. By symbol\n");
    printf("3. By coordinate\n");
    int option = 0;
    if (!prompt_integer_range("Choose search mode (1-3): ", &option, 1, 3)) {
        printf("Search selection invalid.\n");
        return;
    }
    switch (option) {
        case 1: search_by_type(); break;
        case 2: search_by_symbol(); break;
        case 3: search_by_coordinate(); break;
        default: printf("Unknown search option.\n"); break;
    }
}

static void action_add_object(void) {
    printf("\n=== Add New Object ===\n");
    printf("1. Circle\n2. Rectangle\n3. Line\n4. Triangle\n");
    int typeSelection = 0;
    if (!prompt_integer_range("Select shape type (1-4): ", &typeSelection, 1, 4)) {
        printf("Invalid type.\n");
        return;
    }
    Shape candidate;
    if (!create_shape_for_kind((ShapeKind)typeSelection, &candidate)) {
        printf("Could not create shape.\n");
        return;
    }
    if (!add_shape_to_drawing(&candidate)) {
        printf("Object list is full.\n");
        return;
    }
    history_push();
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_delete_object(void) {
    if (activeDrawing.count == 0) {
        printf("No objects to delete.\n");
        return;
    }
    list_all_shapes();
    int index = 0;
    if (!prompt_integer_range("Enter index of object to delete: ", &index, 0, activeDrawing.count - 1)) {
        printf("Invalid index.\n");
        return;
    }
    if (!delete_shape_from_drawing(index)) {
        printf("Could not delete object.\n");
        return;
    }
    history_push();
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_view_canvas(void) {
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_list_objects(void) {
    list_all_shapes();
}

static void action_clear_all(void) {
    if (activeDrawing.count == 0) {
        printf("Canvas is already empty.\n");
        return;
    }
    clear_drawing();
    history_push();
    refresh_canvas(&activeCanvas, &activeDrawing);
    printf("All objects removed.\n");
}

static void action_pick_set_point(void) {
    Point location;
    if (!prompt_integer_range("Enter point X (0..39): ", &location.x, 0, CANVAS_WIDTH - 1) ||
        !prompt_integer_range("Enter point Y (0..19): ", &location.y, 0, CANVAS_HEIGHT - 1)) {
        printf("Invalid coordinates.\n");
        return;
    }
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
    printf("Current character at (%d,%d): %c\n", location.x, location.y, activeCanvas.cells[location.y][location.x]);
    if (!prompt_yes_no("Do you want to set a new character there? (y/n): ")) {
        return;
    }
    char symbol = '\0';
    if (!prompt_char("Enter replacement character: ", &symbol)) {
        printf("Invalid symbol.\n");
        return;
    }
    Shape pointShape = {
        .kind = SHAPE_LINE,
        .p1 = location,
        .p2 = location,
        .p3 = {0, 0},
        .radius = 0,
        .glyph = symbol,
        .filled = false
    };
    if (!add_shape_to_drawing(&pointShape)) {
        printf("Cannot add point object because storage is full.\n");
        return;
    }
    history_push();
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_modify_object(void) {
    if (activeDrawing.count == 0) {
        printf("No objects available to modify.\n");
        return;
    }
    list_all_shapes();
    int index = 0;
    if (!prompt_integer_range("Enter object index to modify: ", &index, 0, activeDrawing.count - 1)) {
        printf("Invalid index.\n");
        return;
    }
    history_push();
    replace_shape_fields(&activeDrawing.items[index]);
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_move_object(void) {
    if (activeDrawing.count == 0) {
        printf("No objects available to move.\n");
        return;
    }
    list_all_shapes();
    int index = 0;
    if (!prompt_integer_range("Enter object index to move: ", &index, 0, activeDrawing.count - 1)) {
        printf("Invalid index.\n");
        return;
    }
    int dx = 0, dy = 0;
    if (!prompt_integer_range("Enter horizontal shift (-40..40): ", &dx, -40, 40) ||
        !prompt_integer_range("Enter vertical shift (-20..20): ", &dy, -20, 20)) {
        printf("Invalid movement values.\n");
        return;
    }
    history_push();
    if (!move_shape_in_drawing(index, dx, dy)) {
        printf("Move failed.\n");
        return;
    }
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_duplicate_object(void) {
    if (activeDrawing.count == 0) {
        printf("No objects to duplicate.\n");
        return;
    }
    list_all_shapes();
    int index = 0;
    if (!prompt_integer_range("Enter object index to duplicate: ", &index, 0, activeDrawing.count - 1)) {
        printf("Invalid index.\n");
        return;
    }
    if (!duplicate_shape_in_drawing(index)) {
        printf("Duplicate failed.\n");
        return;
    }
    history_push();
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
}

static void action_search_objects(void) {
    perform_object_search();
}

static void action_canvas_statistics(void) {
    show_canvas_statistics();
}

static void action_save_drawing(void) {
    char filename[INPUT_BUFFER];
    if (!prompt_string("Enter save filename: ", filename, sizeof(filename))) {
        printf("Failed to read filename.\n");
        return;
    }
    if (!save_drawing_to_file(filename)) {
        printf("Save failed. Check file name and permissions.\n");
        return;
    }
    printf("Drawing saved to %s\n", filename);
}

static void action_load_drawing(void) {
    char filename[INPUT_BUFFER];
    if (!prompt_string("Enter load filename: ", filename, sizeof(filename))) {
        printf("Failed to read filename.\n");
        return;
    }
    if (!load_drawing_from_file(filename)) {
        printf("Load failed. File may be invalid.\n");
        return;
    }
    history_push();
    refresh_canvas(&activeCanvas, &activeDrawing);
    display_canvas(&activeCanvas);
    printf("Drawing loaded from %s\n", filename);
}

static void action_export_ascii(void) {
    char filename[INPUT_BUFFER];
    if (!prompt_string("Enter ASCII export filename: ", filename, sizeof(filename))) {
        printf("Failed to read filename.\n");
        return;
    }
    refresh_canvas(&activeCanvas, &activeDrawing);
    if (!export_canvas_ascii(filename)) {
        printf("Export failed.\n");
        return;
    }
    printf("ASCII art exported to %s\n", filename);
}

static void action_undo(void) {
    history_undo();
    display_canvas(&activeCanvas);
}

static void action_redo(void) {
    history_redo();
    display_canvas(&activeCanvas);
}

static void action_measure_shape(void) {
    if (activeDrawing.count == 0) {
        printf("No objects available for measurement.\n");
        return;
    }
    list_all_shapes();
    int index = 0;
    if (!prompt_integer_range("Enter object index for measurement: ", &index, 0, activeDrawing.count - 1)) {
        printf("Invalid index.\n");
        return;
    }
    const Shape *shape = &activeDrawing.items[index];
    double area = shape_area(shape);
    double perimeter = shape_perimeter(shape);
    printf("Object %d area = %.2f, perimeter = %.2f\n", index, area, perimeter);
}

static void print_menu(void) {
    printf("\n=== 2D Graphics Editor ===\n");
    printf("1  Add Object\n");
    printf("2  Delete Object\n");
    printf("3  View Canvas\n");
    printf("4  List Objects\n");
    printf("5  Clear All Objects\n");
    printf("6  Pick/Set Point\n");
    printf("7  Modify Object\n");
    printf("8  Move Object\n");
    printf("9  Duplicate Object\n");
    printf("10 Search Objects\n");
    printf("11 Canvas Statistics\n");
    printf("12 Save Drawing\n");
    printf("13 Load Drawing\n");
    printf("14 Export ASCII Art\n");
    printf("15 Undo\n");
    printf("16 Redo\n");
    printf("17 Area / Perimeter\n");
    printf("18 Exit\n");
}

static void run_editor(void) {
    bool exitRequested = false;
    while (!exitRequested) {
        print_menu();
        int choice = 0;
        if (!prompt_integer_range("Choose an option (1-18): ", &choice, 1, 18)) {
            printf("Please enter a valid option.\n");
            continue;
        }
        switch (choice) {
            case 1: action_add_object(); break;
            case 2: action_delete_object(); break;
            case 3: action_view_canvas(); break;
            case 4: action_list_objects(); break;
            case 5: action_clear_all(); break;
            case 6: action_pick_set_point(); break;
            case 7: action_modify_object(); break;
            case 8: action_move_object(); break;
            case 9: action_duplicate_object(); break;
            case 10: action_search_objects(); break;
            case 11: action_canvas_statistics(); break;
            case 12: action_save_drawing(); break;
            case 13: action_load_drawing(); break;
            case 14: action_export_ascii(); break;
            case 15: action_undo(); break;
            case 16: action_redo(); break;
            case 17: action_measure_shape(); break;
            case 18:
                exitRequested = true;
                printf("Exiting editor.\n");
                break;
            default:
                printf("Unknown action.\n");
                break;
        }
    }
}

int main(void) {
    canvas_clear(&activeCanvas);
    activeDrawing.count = 0;
    history_initialize();
    run_editor();
    return 0;
}

