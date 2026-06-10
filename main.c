#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define DEFAULT_WIDTH 60
#define DEFAULT_HEIGHT 20
#define MAX_SHAPES 100

// ANSI color codes for enhanced terminal UX
#define CLEAR_SCREEN "\033[H\033[J"
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define CYAN "\033[1;36m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define MAGENTA "\033[1;35m"
#define GREY "\033[90m"

// Shape types enumeration
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

// Shape coordinates and parameter representations
typedef struct {
    int x1, y1, x2, y2;
} LineParams;

typedef struct {
    int x1, y1, x2, y2;
} RectParams;

typedef struct {
    int cx, cy, r;
} CircleParams;

typedef struct {
    int x1, y1, x2, y2, x3, y3;
} TriParams;

// Unified shape structure
typedef struct {
    int id;
    ShapeType type;
    char draw_char;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriParams triangle;
    } params;
} Shape;

// Canvas state variables
int canvas_width = DEFAULT_WIDTH;
int canvas_height = DEFAULT_HEIGHT;
char bg_char = '_';
char **grid = NULL;

Shape shapes[MAX_SHAPES];
int shape_count = 0;
int next_id = 1;

// --- Canvas Memory Operations ---

void free_grid() {
    if (grid != NULL) {
        for (int i = 0; i < canvas_height; i++) {
            free(grid[i]);
        }
        free(grid);
        grid = NULL;
    }
}

void init_grid() {
    free_grid();
    grid = (char **)malloc(canvas_height * sizeof(char *));
    for (int i = 0; i < canvas_height; i++) {
        grid[i] = (char *)malloc(canvas_width * sizeof(char));
        for (int j = 0; j < canvas_width; j++) {
            grid[i][j] = bg_char;
        }
    }
}

void clear_grid() {
    for (int i = 0; i < canvas_height; i++) {
        for (int j = 0; j < canvas_width; j++) {
            grid[i][j] = bg_char;
        }
    }
}

// --- Screen Clearing and Colors ---

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    printf(CLEAR_SCREEN);
    fflush(stdout);
#endif
}

// Enable ANSI virtual terminal processing on Windows if applicable
void enable_ansi_support() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

// --- Drawing Algorithms ---

void plot_point(int x, int y, char draw_char) {
    if (x >= 0 && x < canvas_width && y >= 0 && y < canvas_height) {
        grid[y][x] = draw_char;
    }
}

void draw_line(int x1, int y1, int x2, int y2, char draw_char) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        plot_point(x1, y1, draw_char);
        if (x1 == x2 && y1 == y2) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void draw_circle(int cx, int cy, int r, char draw_char) {
    int x = r;
    int y = 0;
    int err = 1 - r;

    while (x >= y) {
        plot_point(cx + x, cy + y, draw_char);
        plot_point(cx - x, cy + y, draw_char);
        plot_point(cx + x, cy - y, draw_char);
        plot_point(cx - x, cy - y, draw_char);
        plot_point(cx + y, cy + x, draw_char);
        plot_point(cx - y, cy + x, draw_char);
        plot_point(cx + y, cy - x, draw_char);
        plot_point(cx - y, cy - x, draw_char);

        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void draw_rectangle(int x1, int y1, int x2, int y2, char draw_char) {
    draw_line(x1, y1, x2, y1, draw_char);
    draw_line(x1, y2, x2, y2, draw_char);
    draw_line(x1, y1, x1, y2, draw_char);
    draw_line(x2, y1, x2, y2, draw_char);
}

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, char draw_char) {
    draw_line(x1, y1, x2, y2, draw_char);
    draw_line(x2, y2, x3, y3, draw_char);
    draw_line(x3, y3, x1, y1, draw_char);
}

// Draw shapes array onto clean grid
void render_all() {
    clear_grid();
    for (int i = 0; i < shape_count; i++) {
        Shape s = shapes[i];
        if (s.type == SHAPE_LINE) {
            draw_line(s.params.line.x1, s.params.line.y1, s.params.line.x2, s.params.line.y2, s.draw_char);
        } else if (s.type == SHAPE_RECTANGLE) {
            draw_rectangle(s.params.rect.x1, s.params.rect.y1, s.params.rect.x2, s.params.rect.y2, s.draw_char);
        } else if (s.type == SHAPE_CIRCLE) {
            draw_circle(s.params.circle.cx, s.params.circle.cy, s.params.circle.r, s.draw_char);
        } else if (s.type == SHAPE_TRIANGLE) {
            draw_triangle(s.params.triangle.x1, s.params.triangle.y1, s.params.triangle.x2, s.params.triangle.y2, s.params.triangle.x3, s.params.triangle.y3, s.draw_char);
        }
    }
}

// Display grid structure in terminal
void display_canvas() {
    // Column tens headers
    printf("   ");
    for (int j = 0; j < canvas_width; j++) {
        if (j >= 10 && j % 10 == 0) {
            printf(GREY "%d" RESET, j / 10);
        } else {
            printf(" ");
        }
    }
    printf("\n");

    // Column ones headers
    printf("   ");
    for (int j = 0; j < canvas_width; j++) {
        printf(GREY "%d" RESET, j % 10);
    }
    printf("\n");

    // Top border line
    printf("  " CYAN "+");
    for (int j = 0; j < canvas_width; j++) printf("-");
    printf("+" RESET "\n");

    // Frame rows
    for (int i = 0; i < canvas_height; i++) {
        printf(GREY "%2d" RESET CYAN "|" RESET, i);
        for (int j = 0; j < canvas_width; j++) {
            printf("%c", grid[i][j]);
        }
        printf(CYAN "|" RESET GREY "%d" RESET "\n", i);
    }

    // Bottom border line
    printf("  " CYAN "+");
    for (int j = 0; j < canvas_width; j++) printf("-");
    printf("+" RESET "\n");
}

// --- Object CRUD Logic ---

int add_shape(Shape s) {
    if (shape_count >= MAX_SHAPES) {
        return -1;
    }
    s.id = next_id++;
    shapes[shape_count++] = s;
    return s.id;
}

int delete_shape(int id) {
    for (int i = 0; i < shape_count; i++) {
        if (shapes[i].id == id) {
            for (int j = i; j < shape_count - 1; j++) {
                shapes[j] = shapes[j + 1];
            }
            shape_count--;
            return 1;
        }
    }
    return 0;
}

Shape* find_shape(int id) {
    for (int i = 0; i < shape_count; i++) {
        if (shapes[i].id == id) {
            return &shapes[i];
        }
    }
    return NULL;
}

void print_shape(Shape s) {
    printf(BOLD CYAN "[ID: %d]" RESET " ", s.id);
    if (s.type == SHAPE_LINE) {
        printf("Line: (%d, %d) to (%d, %d) using '%c'", s.params.line.x1, s.params.line.y1, s.params.line.x2, s.params.line.y2, s.draw_char);
    } else if (s.type == SHAPE_RECTANGLE) {
        printf("Rectangle: Top-Left (%d, %d), Bottom-Right (%d, %d) using '%c'", s.params.rect.x1, s.params.rect.y1, s.params.rect.x2, s.params.rect.y2, s.draw_char);
    } else if (s.type == SHAPE_CIRCLE) {
        printf("Circle: Center (%d, %d), Radius %d using '%c'", s.params.circle.cx, s.params.circle.cy, s.params.circle.r, s.draw_char);
    } else if (s.type == SHAPE_TRIANGLE) {
        printf("Triangle: P1(%d, %d), P2(%d, %d), P3(%d, %d) using '%c'", s.params.triangle.x1, s.params.triangle.y1, s.params.triangle.x2, s.params.triangle.y2, s.params.triangle.x3, s.params.triangle.y3, s.draw_char);
    }
    printf("\n");
}

// --- Text File Saving & Loading ---

int save_to_file(const char *filepath) {
    FILE *fp = fopen(filepath, "w");
    if (!fp) return 0;

    fprintf(fp, "%d %d %c %d %d\n", canvas_width, canvas_height, bg_char, next_id, shape_count);
    for (int i = 0; i < shape_count; i++) {
        Shape s = shapes[i];
        fprintf(fp, "%d %d %c ", s.id, (int)s.type, s.draw_char);
        if (s.type == SHAPE_LINE) {
            fprintf(fp, "%d %d %d %d\n", s.params.line.x1, s.params.line.y1, s.params.line.x2, s.params.line.y2);
        } else if (s.type == SHAPE_RECTANGLE) {
            fprintf(fp, "%d %d %d %d\n", s.params.rect.x1, s.params.rect.y1, s.params.rect.x2, s.params.rect.y2);
        } else if (s.type == SHAPE_CIRCLE) {
            fprintf(fp, "%d %d %d\n", s.params.circle.cx, s.params.circle.cy, s.params.circle.r);
        } else if (s.type == SHAPE_TRIANGLE) {
            fprintf(fp, "%d %d %d %d %d %d\n", s.params.triangle.x1, s.params.triangle.y1, s.params.triangle.x2, s.params.triangle.y2, s.params.triangle.x3, s.params.triangle.y3);
        }
    }
    fclose(fp);
    return 1;
}

int load_from_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return 0;

    int loaded_count = 0;
    if (fscanf(fp, "%d %d %c %d %d\n", &canvas_width, &canvas_height, &bg_char, &next_id, &loaded_count) != 5) {
        fclose(fp);
        return 0;
    }

    init_grid(); // Allocate correct grid capacity
    shape_count = 0;

    for (int i = 0; i < loaded_count; i++) {
        Shape s;
        int type_int;
        if (fscanf(fp, "%d %d %c ", &s.id, &type_int, &s.draw_char) != 3) {
            break;
        }
        s.type = (ShapeType)type_int;

        if (s.type == SHAPE_LINE) {
            fscanf(fp, "%d %d %d %d\n", &s.params.line.x1, &s.params.line.y1, &s.params.line.x2, &s.params.line.y2);
        } else if (s.type == SHAPE_RECTANGLE) {
            fscanf(fp, "%d %d %d %d\n", &s.params.rect.x1, &s.params.rect.y1, &s.params.rect.x2, &s.params.rect.y2);
        } else if (s.type == SHAPE_CIRCLE) {
            fscanf(fp, "%d %d %d\n", &s.params.circle.cx, &s.params.circle.cy, &s.params.circle.r);
        } else if (s.type == SHAPE_TRIANGLE) {
            fscanf(fp, "%d %d %d %d %d %d\n", &s.params.triangle.x1, &s.params.triangle.y1, &s.params.triangle.x2, &s.params.triangle.y2, &s.params.triangle.x3, &s.params.triangle.y3);
        }

        if (shape_count < MAX_SHAPES) {
            shapes[shape_count++] = s;
        }
    }
    fclose(fp);
    return 1;
}

// --- Safe User Console Input Helpers ---

int prompt_int(const char *prompt, int default_val) {
    char buffer[100];
    printf("%s [%d]: ", prompt, default_val);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return default_val;
    }
    // Trim trailing newline character
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) == 0) {
        return default_val;
    }
    char *endptr;
    long val = strtol(buffer, &endptr, 10);
    if (endptr == buffer) {
        return default_val;
    }
    return (int)val;
}

char prompt_char(const char *prompt, char default_val) {
    char buffer[100];
    printf("%s [%c]: ", prompt, default_val);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return default_val;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) == 0) {
        return default_val;
    }
    return buffer[0];
}

void prompt_string(const char *prompt, const char *default_val, char *out_str, int max_len) {
    printf("%s [%s]: ", prompt, default_val);
    if (fgets(out_str, max_len, stdin) == NULL) {
        strcpy(out_str, default_val);
        return;
    }
    out_str[strcspn(out_str, "\n")] = '\0';
    if (strlen(out_str) == 0) {
        strcpy(out_str, default_val);
    }
}

// --- Main Menu Loop ---

int main() {
    enable_ansi_support();
    init_grid();

    char message[256];
    strcpy(message, "Welcome to the C 2D Graphics Editor!");
    int is_error = 0;

    while (1) {
        clear_screen();
        printf(BOLD MAGENTA "=================================================\n" RESET);
        printf(BOLD MAGENTA "   🎨 2D C CONSOLE GRAPHICS EDITOR (* & _)\n" RESET);
        printf(BOLD MAGENTA "=================================================\n" RESET);

        if (strlen(message) > 0) {
            if (is_error) {
                printf(RED "[ERROR] %s\n" RESET, message);
            } else {
                printf(GREEN "[INFO] %s\n" RESET, message);
            }
            message[0] = '\0';
            is_error = 0;
        }

        render_all();
        display_canvas();

        // Active Objects list view
        printf("\n" BOLD YELLOW "--- Active Objects List ---\n" RESET);
        if (shape_count == 0) {
            printf(GREY "No objects drawn yet.\n" RESET);
        } else {
            for (int i = 0; i < shape_count; i++) {
                print_shape(shapes[i]);
            }
        }

        // Selection prompt menu
        printf("\n" BOLD YELLOW "--- Main Menu ---\n" RESET);
        printf("  " BOLD "1" RESET ". Add Shape\n");
        printf("  " BOLD "2" RESET ". Delete Shape\n");
        printf("  " BOLD "3" RESET ". Modify Shape\n");
        printf("  " BOLD "4" RESET ". Clear Canvas\n");
        printf("  " BOLD "5" RESET ". Resize Canvas (Current: %dx%d)\n", canvas_width, canvas_height);
        printf("  " BOLD "6" RESET ". Save Canvas to File\n");
        printf("  " BOLD "7" RESET ". Load Canvas from File\n");
        printf("  " BOLD "0" RESET ". Exit Editor\n");

        int choice = prompt_int("\nSelect an option (0-7)", -1);

        if (choice == 0) {
            printf("\n" GREEN "Thank you for using the C 2D Graphics Editor! Goodbye!\n" RESET);
            break;
        } else if (choice == 1) {
            // Add shape
            clear_screen();
            printf(BOLD MAGENTA "--- Add a New Shape ---\n" RESET);
            printf("  1. Line\n");
            printf("  2. Rectangle\n");
            printf("  3. Circle\n");
            printf("  4. Triangle\n");
            printf("  0. Back to main menu\n");

            int shape_choice = prompt_int("\nSelect shape type (0-4)", 0);
            if (shape_choice == 1) {
                Shape s;
                s.type = SHAPE_LINE;
                printf("\n" CYAN "Define Line coordinates:\n" RESET);
                s.params.line.x1 = prompt_int("  Start X", 0);
                s.params.line.y1 = prompt_int("  Start Y", 0);
                s.params.line.x2 = prompt_int("  End X", 10);
                s.params.line.y2 = prompt_int("  End Y", 10);
                s.draw_char = prompt_char("  Drawing character", '*');
                if (add_shape(s) != -1) {
                    strcpy(message, "Line added successfully!");
                } else {
                    strcpy(message, "Error adding shape: Max shapes limit reached.");
                    is_error = 1;
                }
            } else if (shape_choice == 2) {
                Shape s;
                s.type = SHAPE_RECTANGLE;
                printf("\n" CYAN "Define Rectangle coordinates:\n" RESET);
                s.params.rect.x1 = prompt_int("  Top-Left X", 0);
                s.params.rect.y1 = prompt_int("  Top-Left Y", 0);
                s.params.rect.x2 = prompt_int("  Bottom-Right X", 15);
                s.params.rect.y2 = prompt_int("  Bottom-Right Y", 8);
                s.draw_char = prompt_char("  Drawing character", '*');
                if (add_shape(s) != -1) {
                    strcpy(message, "Rectangle added successfully!");
                } else {
                    strcpy(message, "Error adding shape: Max shapes limit reached.");
                    is_error = 1;
                }
            } else if (shape_choice == 3) {
                Shape s;
                s.type = SHAPE_CIRCLE;
                printf("\n" CYAN "Define Circle parameters:\n" RESET);
                s.params.circle.cx = prompt_int("  Center X", 15);
                s.params.circle.cy = prompt_int("  Center Y", 10);
                s.params.circle.r = prompt_int("  Radius", 5);
                s.draw_char = prompt_char("  Drawing character", '*');
                if (add_shape(s) != -1) {
                    strcpy(message, "Circle added successfully!");
                } else {
                    strcpy(message, "Error adding shape: Max shapes limit reached.");
                    is_error = 1;
                }
            } else if (shape_choice == 4) {
                Shape s;
                s.type = SHAPE_TRIANGLE;
                printf("\n" CYAN "Define Triangle coordinates:\n" RESET);
                s.params.triangle.x1 = prompt_int("  Vertex 1 X", 10);
                s.params.triangle.y1 = prompt_int("  Vertex 1 Y", 2);
                s.params.triangle.x2 = prompt_int("  Vertex 2 X", 5);
                s.params.triangle.y2 = prompt_int("  Vertex 2 Y", 10);
                s.params.triangle.x3 = prompt_int("  Vertex 3 X", 15);
                s.params.triangle.y3 = prompt_int("  Vertex 3 Y", 10);
                s.draw_char = prompt_char("  Drawing character", '*');
                if (add_shape(s) != -1) {
                    strcpy(message, "Triangle added successfully!");
                } else {
                    strcpy(message, "Error adding shape: Max shapes limit reached.");
                    is_error = 1;
                }
            }
        } else if (choice == 2) {
            if (shape_count == 0) {
                strcpy(message, "There are no shapes to delete.");
                is_error = 1;
                continue;
            }
            int delete_id = prompt_int("\nEnter Shape ID to delete", -1);
            if (delete_shape(delete_id)) {
                sprintf(message, "Shape %d deleted successfully!", delete_id);
            } else {
                sprintf(message, "Shape with ID %d not found.", delete_id);
                is_error = 1;
            }
        } else if (choice == 3) {
            if (shape_count == 0) {
                strcpy(message, "There are no shapes to modify.");
                is_error = 1;
                continue;
            }
            int modify_id = prompt_int("\nEnter Shape ID to modify", -1);
            Shape *s = find_shape(modify_id);
            if (s == NULL) {
                sprintf(message, "Shape with ID %d not found.", modify_id);
                is_error = 1;
                continue;
            }

            clear_screen();
            printf(BOLD MAGENTA "--- Modify Shape [ID: %d] ---\n" RESET, s->id);
            printf("Current properties: ");
            print_shape(*s);
            printf("\n");

            s->draw_char = prompt_char("  New Drawing character", s->draw_char);

            if (s->type == SHAPE_LINE) {
                s->params.line.x1 = prompt_int("  New Start X", s->params.line.x1);
                s->params.line.y1 = prompt_int("  New Start Y", s->params.line.y1);
                s->params.line.x2 = prompt_int("  New End X", s->params.line.x2);
                s->params.line.y2 = prompt_int("  New End Y", s->params.line.y2);
            } else if (s->type == SHAPE_RECTANGLE) {
                s->params.rect.x1 = prompt_int("  New Top-Left X", s->params.rect.x1);
                s->params.rect.y1 = prompt_int("  New Top-Left Y", s->params.rect.y1);
                s->params.rect.x2 = prompt_int("  New Bottom-Right X", s->params.rect.x2);
                s->params.rect.y2 = prompt_int("  New Bottom-Right Y", s->params.rect.y2);
            } else if (s->type == SHAPE_CIRCLE) {
                s->params.circle.cx = prompt_int("  New Center X", s->params.circle.cx);
                s->params.circle.cy = prompt_int("  New Center Y", s->params.circle.cy);
                s->params.circle.r = prompt_int("  New Radius", s->params.circle.r);
            } else if (s->type == SHAPE_TRIANGLE) {
                s->params.triangle.x1 = prompt_int("  New Vertex 1 X", s->params.triangle.x1);
                s->params.triangle.y1 = prompt_int("  New Vertex 1 Y", s->params.triangle.y1);
                s->params.triangle.x2 = prompt_int("  New Vertex 2 X", s->params.triangle.x2);
                s->params.triangle.y2 = prompt_int("  New Vertex 2 Y", s->params.triangle.y2);
                s->params.triangle.x3 = prompt_int("  New Vertex 3 X", s->params.triangle.x3);
                s->params.triangle.y3 = prompt_int("  New Vertex 3 Y", s->params.triangle.y3);
            }
            sprintf(message, "Shape %d modified successfully!", modify_id);
        } else if (choice == 4) {
            char confirm = prompt_char("\nAre you sure you want to clear the canvas? (y/n)", 'n');
            if (confirm == 'y' || confirm == 'Y') {
                shape_count = 0;
                next_id = 1;
                strcpy(message, "Canvas cleared successfully!");
            } else {
                strcpy(message, "Clear canvas cancelled.");
            }
        } else if (choice == 5) {
            printf("\n" CYAN "Resize canvas boundaries (Current size: %dx%d)\n" RESET, canvas_width, canvas_height);
            int new_w = prompt_int("  New Width", canvas_width);
            int new_h = prompt_int("  New Height", canvas_height);
            if (new_w <= 0 || new_h <= 0) {
                strcpy(message, "Width and Height must be positive integers.");
                is_error = 1;
            } else {
                canvas_width = new_w;
                canvas_height = new_h;
                init_grid();
                sprintf(message, "Canvas resized to %dx%d!", new_w, new_h);
            }
        } else if (choice == 6) {
            char filepath[256];
            prompt_string("\nEnter filename to save", "picture.txt", filepath, sizeof(filepath));
            if (save_to_file(filepath)) {
                sprintf(message, "Canvas successfully saved to '%s'!", filepath);
            } else {
                sprintf(message, "Failed to save to file '%s'.", filepath);
                is_error = 1;
            }
        } else if (choice == 7) {
            char filepath[256];
            prompt_string("\nEnter filename to load", "picture.txt", filepath, sizeof(filepath));
            if (load_from_file(filepath)) {
                sprintf(message, "Canvas successfully loaded from '%s'!", filepath);
            } else {
                sprintf(message, "Failed to load from file '%s'.", filepath);
                is_error = 1;
            }
        } else {
            strcpy(message, "Invalid menu choice. Please select an option between 0 and 7.");
            is_error = 1;
        }
    }

    free_grid();
    return 0;
}
