#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CANVAS_WIDTH 40
#define CANVAS_HEIGHT 20
#define MAX_OBJECTS 100

// Shape types
typedef enum {
    CIRCLE,
    RECTANGLE,
    LINE,
    TRIANGLE
} ShapeType;

// Shape structure
typedef struct {
    ShapeType type;
    int x1, y1, x2, y2, x3, y3;
    int radius;
    char symbol;
} Shape;

// Canvas and objects
char canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
Shape objects[MAX_OBJECTS];
int object_count = 0;

// Initialize canvas with underscores
void init_canvas() {
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            canvas[y][x] = '_';
        }
    }
}

// Input helpers
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

// Draw a point on canvas
void draw_point(int x, int y, char symbol) {
    if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
        canvas[y][x] = symbol;
    }
}

// Draw a line using Bresenham's algorithm
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

// Draw a rectangle
void draw_rectangle(int x1, int y1, int x2, int y2, char symbol) {
    // Normalize coordinates
    int min_x = (x1 < x2) ? x1 : x2;
    int max_x = (x1 < x2) ? x2 : x1;
    int min_y = (y1 < y2) ? y1 : y2;
    int max_y = (y1 < y2) ? y2 : y1;

    // Draw top and bottom
    for (int x = min_x; x <= max_x; x++) {
        draw_point(x, min_y, symbol);
        draw_point(x, max_y, symbol);
    }

    // Draw left and right
    for (int y = min_y; y <= max_y; y++) {
        draw_point(min_x, y, symbol);
        draw_point(max_x, y, symbol);
    }
}

// Draw a circle using Midpoint Circle Algorithm
void draw_circle(int cx, int cy, int radius, char symbol) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        // Draw 8 symmetrical points
        draw_point(cx + x, cy + y, symbol);
        draw_point(cx - x, cy + y, symbol);
        draw_point(cx + x, cy - y, symbol);
        draw_point(cx - x, cy - y, symbol);
        draw_point(cx + y, cy + x, symbol);
        draw_point(cx - y, cy + x, symbol);
        draw_point(cx + y, cy - x, symbol);
        draw_point(cx - y, cy - x, symbol);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

// Draw a triangle
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, char symbol) {
    draw_line(x1, y1, x2, y2, symbol);
    draw_line(x2, y2, x3, y3, symbol);
    draw_line(x3, y3, x1, y1, symbol);
}

// Add a shape to the objects list
int add_object(ShapeType type, int x1, int y1, int x2, int y2, int x3, int y3, int radius, char symbol) {
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

    printf("Object %d added successfully!\n", object_count);
    return object_count++;
}

// Delete an object from the list
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

// Modify an existing object
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

// Rebuild the canvas with '_' and draw all objects
void rebuild_canvas() {
    init_canvas();

    for (int i = 0; i < object_count; i++) {
        Shape *shape = &objects[i];
        switch (shape->type) {
            case CIRCLE:
                draw_circle(shape->x1, shape->y1, shape->radius, shape->symbol);
                break;
            case RECTANGLE:
                draw_rectangle(shape->x1, shape->y1, shape->x2, shape->y2, shape->symbol);
                break;
            case LINE:
                draw_line(shape->x1, shape->y1, shape->x2, shape->y2, shape->symbol);
                break;
            case TRIANGLE:
                draw_triangle(shape->x1, shape->y1, shape->x2, shape->y2, shape->x3, shape->y3, shape->symbol);
                break;
        }
    }
}

// Display the canvas
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

// Allow the user to pick a point from a canvas row and optionally set it
void pick_point() {
    int y, x;
    printf("\n=== Pick/Set Point ===\n");
    if (!read_int("Enter row (0..%d): ", &y)) { printf("Invalid input.\n"); return; }
    if (y < 0 || y >= CANVAS_HEIGHT) { printf("Row out of range.\n"); return; }

    // Show an index ruler and the selected row
    printf("Index: ");
    for (int i = 0; i < CANVAS_WIDTH; i++) {
        printf("%d", i % 10);
    }
    printf("\n");

    printf("Row %d:  ", y);
    for (int i = 0; i < CANVAS_WIDTH; i++) putchar(canvas[y][i]);
    printf("\n");

    if (!read_int("Enter column (0..%d): ", &x)) { printf("Invalid input.\n"); return; }
    if (x < 0 || x >= CANVAS_WIDTH) { printf("Column out of range.\n"); return; }

    printf("Character at (%d,%d) = '%c'\n", x, y, canvas[y][x]);
    char ans = 'n';
    if (!read_char("Do you want to change it? (y/n): ", &ans)) { printf("Invalid input.\n"); return; }
    if (ans == 'y' || ans == 'Y') {
        char symbol;
        if (!read_char("Enter new symbol: ", &symbol)) { printf("Invalid input.\n"); return; }
        // Add a tiny object representing a single point so it persists when canvas rebuilt
        add_object(LINE, x, y, x, y, 0, 0, 0, symbol);
        printf("Point set at (%d,%d) with symbol '%c'.\n", x, y, symbol);
    } else {
        printf("No change made.\n");
    }
}

// List all objects
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
                printf("Circle at (%d, %d) radius=%d symbol='%c'\n", 
                    objects[i].x1, objects[i].y1, objects[i].radius, objects[i].symbol);
                break;
            case RECTANGLE:
                printf("Rectangle from (%d, %d) to (%d, %d) symbol='%c'\n", 
                    objects[i].x1, objects[i].y1, objects[i].x2, objects[i].y2, objects[i].symbol);
                break;
            case LINE:
                printf("Line from (%d, %d) to (%d, %d) symbol='%c'\n", 
                    objects[i].x1, objects[i].y1, objects[i].x2, objects[i].y2, objects[i].symbol);
                break;
            case TRIANGLE:
                printf("Triangle (%d, %d) (%d, %d) (%d, %d) symbol='%c'\n", 
                    objects[i].x1, objects[i].y1, objects[i].x2, objects[i].y2, objects[i].x3, objects[i].y3, objects[i].symbol);
                break;
        }
    }
    printf("\n");
}

// Menu for adding objects
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

    switch (choice) {
        case 1:
            if (!read_int("Enter center X: ", &x1) || !read_int("Enter center Y: ", &y1)) { printf("Invalid input.\n"); return; }
            if (!read_int("Enter radius: ", &radius)) { printf("Invalid input.\n"); return; }
            add_object(CIRCLE, x1, y1, 0, 0, 0, 0, radius, symbol);
            break;
        case 2:
            if (!read_int("Enter top-left X: ", &x1) || !read_int("Enter top-left Y: ", &y1) || !read_int("Enter bottom-right X: ", &x2) || !read_int("Enter bottom-right Y: ", &y2)) { printf("Invalid input.\n"); return; }
            add_object(RECTANGLE, x1, y1, x2, y2, 0, 0, 0, symbol);
            break;
        case 3:
            if (!read_int("Enter start X: ", &x1) || !read_int("Enter start Y: ", &y1) || !read_int("Enter end X: ", &x2) || !read_int("Enter end Y: ", &y2)) { printf("Invalid input.\n"); return; }
            add_object(LINE, x1, y1, x2, y2, 0, 0, 0, symbol);
            break;
        case 4:
            if (!read_int("Enter x1: ", &x1) || !read_int("Enter y1: ", &y1) || !read_int("Enter x2: ", &x2) || !read_int("Enter y2: ", &y2) || !read_int("Enter x3: ", &x3) || !read_int("Enter y3: ", &y3)) { printf("Invalid input.\n"); return; }
            add_object(TRIANGLE, x1, y1, x2, y2, x3, y3, 0, symbol);
            break;
        default:
            printf("Invalid choice!\n");
    }
}

// Main menu
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
        printf("8. Exit\n");
        printf("Select (1-8): ");
        
        if (!read_int(NULL, &choice)) { printf("Invalid input.\n"); continue; }

        switch (choice) {
            case 1:
                menu_add_object();
                rebuild_canvas();
                display_canvas();
                break;
            case 2: {
                list_objects();
                if (object_count > 0) {
                    int index;
                    if (!read_int("Enter object index to delete: ", &index)) { printf("Invalid input.\n"); break; }
                    delete_object(index);
                    rebuild_canvas();
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
                object_count = 0;
                init_canvas();
                printf("All objects cleared!\n");
                break;
            case 6:
                pick_point();
                rebuild_canvas();
                display_canvas();
                break;
            case 7: {
                list_objects();
                if (object_count > 0) {
                    int index;
                    if (!read_int("Enter object index to modify: ", &index)) { printf("Invalid input.\n"); break; }
                    modify_object(index);
                    rebuild_canvas();
                    display_canvas();
                }
                break;
            }
            case 8:
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

