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
    printf("Enter new symbol (or same symbol to keep): ");
    scanf(" %c", &shape->symbol);

    switch (shape->type) {
        case CIRCLE:
            printf("Enter new center X Y and radius: ");
            scanf("%d %d %d", &shape->x1, &shape->y1, &shape->radius);
            break;
        case RECTANGLE:
            printf("Enter new top-left X Y and bottom-right X Y: ");
            scanf("%d %d %d %d", &shape->x1, &shape->y1, &shape->x2, &shape->y2);
            break;
        case LINE:
            printf("Enter new start X Y and end X Y: ");
            scanf("%d %d %d %d", &shape->x1, &shape->y1, &shape->x2, &shape->y2);
            break;
        case TRIANGLE:
            printf("Enter new three points x1 y1 x2 y2 x3 y3: ");
            scanf("%d %d %d %d %d %d", &shape->x1, &shape->y1, &shape->x2, &shape->y2, &shape->x3, &shape->y3);
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
    printf("Enter row (0..%d): ", CANVAS_HEIGHT - 1);
    if (scanf("%d", &y) != 1) { scanf("%*s"); printf("Invalid input.\n"); return; }
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

    printf("Enter column (0..%d): ", CANVAS_WIDTH - 1);
    if (scanf("%d", &x) != 1) { scanf("%*s"); printf("Invalid input.\n"); return; }
    if (x < 0 || x >= CANVAS_WIDTH) { printf("Column out of range.\n"); return; }

    printf("Character at (%d,%d) = '%c'\n", x, y, canvas[y][x]);
    printf("Do you want to change it? (y/n): ");
    char ans = 'n';
    scanf(" %c", &ans);
    if (ans == 'y' || ans == 'Y') {
        char symbol;
        printf("Enter new symbol: ");
        scanf(" %c", &symbol);
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
    scanf("%d", &choice);

    char symbol;
    printf("Enter symbol (e.g., *, _, #): ");
    scanf(" %c", &symbol);

    int x1, y1, x2, y2, x3, y3, radius;

    switch (choice) {
        case 1:
            printf("Enter center X and Y: ");
            scanf("%d %d", &x1, &y1);
            printf("Enter radius: ");
            scanf("%d", &radius);
            add_object(CIRCLE, x1, y1, 0, 0, 0, 0, radius, symbol);
            break;
        case 2:
            printf("Enter top-left X, Y and bottom-right X, Y: ");
            scanf("%d %d %d %d", &x1, &y1, &x2, &y2);
            add_object(RECTANGLE, x1, y1, x2, y2, 0, 0, 0, symbol);
            break;
        case 3:
            printf("Enter start X, Y and end X, Y: ");
            scanf("%d %d %d %d", &x1, &y1, &x2, &y2);
            add_object(LINE, x1, y1, x2, y2, 0, 0, 0, symbol);
            break;
        case 4:
            printf("Enter three points (x1 y1 x2 y2 x3 y3): ");
            scanf("%d %d %d %d %d %d", &x1, &y1, &x2, &y2, &x3, &y3);
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
        
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                menu_add_object();
                rebuild_canvas();
                rebuild_canvas();
                display_canvas();
                break;
            case 2: {
                list_objects();
                if (object_count > 0) {
                    printf("Enter object index to delete: ");
                    int index;
                    scanf("%d", &index);
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
                    printf("Enter object index to modify: ");
                    int index;
                    scanf("%d", &index);
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

