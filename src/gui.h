#ifndef CICEKLISCRIPT_GUI_H
#define CICEKLISCRIPT_GUI_H

#include "value.h"
#include <stdbool.h>

#ifndef NO_X11
#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>
#else
typedef void* Display;
typedef unsigned long Window;
typedef unsigned long Pixmap;
typedef void* GC;
typedef unsigned long Atom;
typedef void* cairo_surface_t;
typedef void* cairo_t;
#endif

typedef struct GuiWindow {
    Obj obj;
    Display* display;
    int screen;
    Window xwindow;
    Pixmap pixmap;
    GC gc;
    Atom wm_delete;
    cairo_surface_t* surface;
    cairo_t* cr;
    int width;
    int height;
    char* title;
    bool is_open;
    double bg_r, bg_g, bg_b;
    int mouse_x;
    int mouse_y;
    bool mouse_down;
    char last_key[32];
    bool headless;
    bool emitted;
    int op_count;
    int op_cap;
    char** ops;
} GuiWindow;

GuiWindow* new_gui_window(int width, int height, const char* title);
void gui_window_destroy(GuiWindow* win);
void gui_cleanup(void);

void gui_window_poll_events(GuiWindow* win);
void gui_window_update(GuiWindow* win);
void gui_window_color(GuiWindow* win, double r, double g, double b);
void gui_window_text(GuiWindow* win, const char* text, double r, double g, double b, double size, double x, double y);
void gui_window_rect(GuiWindow* win, double x, double y, double w, double h, double r, double g, double b);
void gui_window_circle(GuiWindow* win, double x, double y, double radius, double r, double g, double b);
void gui_window_line(GuiWindow* win, double x1, double y1, double x2, double y2, double r, double g, double b, double width);

Value native_mete_window(Value self, size_t argc, Value* args, int line);
Value gui_window_get_method(Value win_val, const char* name);
Value gui_window_get_property(Value win_val, const char* name);

#endif // CICEKLISCRIPT_GUI_H
