#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "gui.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <unistd.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define TokenType WindowsTokenType
#include <windows.h>
#undef TokenType
#endif
#ifndef NO_X11
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <glob.h>
#endif
#include <sys/types.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static bool gui_headless_mode(void) {
    const char* env = getenv("CICEKLI_GUI_HEADLESS");
    if (!env || env[0] == '\0') {
        env = getenv("METE_GUI_HEADLESS");
    }
    if (env != NULL && env[0] != '\0') return true;

#if defined(_WIN32)
    return false;
#elif defined(NO_X11)
    return true;
#else
    return false;
#endif
}

#ifdef _WIN32
static bool win32_class_registered = false;

static LRESULT CALLBACK CicekliWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GuiWindow* win = (GuiWindow*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    switch (uMsg) {
        case WM_CLOSE:
            if (win) win->is_open = false;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (win) {
                win->is_open = false;
                win->hwnd = NULL;
            }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (win && win->hdc_mem) {
                BitBlt(hdc, 0, 0, win->width, win->height, (HDC)win->hdc_mem, 0, 0, SRCCOPY);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE:
            if (win) {
                win->mouse_x = (short)LOWORD(lParam);
                win->mouse_y = (short)HIWORD(lParam);
            }
            return 0;
        case WM_LBUTTONDOWN:
            if (win) {
                win->mouse_down = true;
                win->mouse_x = (short)LOWORD(lParam);
                win->mouse_y = (short)HIWORD(lParam);
            }
            return 0;
        case WM_LBUTTONUP:
            if (win) {
                win->mouse_down = false;
            }
            return 0;
        case WM_KEYDOWN:
            if (win) {
                if (wParam == VK_ESCAPE) {
                    win->is_open = false;
                    DestroyWindow(hwnd);
                    return 0;
                }
                BYTE kstate[256];
                GetKeyboardState(kstate);
                WCHAR wbuf[8];
                int r = ToUnicode((UINT)wParam, (UINT)(HIWORD(lParam) & 0xFF), kstate, wbuf, 8, 0);
                if (r > 0) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, r, win->last_key, (int)sizeof(win->last_key) - 1, NULL, NULL);
                    if (len >= 0 && len < (int)sizeof(win->last_key)) {
                        win->last_key[len] = '\0';
                    }
                }
            }
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
#endif

static void json_escape(const char* s, char* out, size_t out_size) {
    size_t oi = 0;
    for (size_t i = 0; s && s[i] && oi + 7 < out_size; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') {
            out[oi++] = '\\';
            out[oi++] = (char)c;
        } else if (c == '\n') {
            out[oi++] = '\\';
            out[oi++] = 'n';
        } else if (c == '\t') {
            out[oi++] = '\\';
            out[oi++] = 't';
        } else if (c < 0x20) {
            oi += (size_t)snprintf(out + oi, out_size - oi, "\\u%04x", c);
        } else {
            out[oi++] = (char)c;
        }
    }
    out[oi] = '\0';
}

static void gui_record_op(GuiWindow* win, const char* op_json) {
    if (!win || !op_json) return;
    if (win->op_count >= win->op_cap) {
        int nc = win->op_cap > 0 ? win->op_cap * 2 : 16;
        char** no = (char**)realloc(win->ops, sizeof(char*) * (size_t)nc);
        if (!no) return;
        win->ops = no;
        win->op_cap = nc;
    }
    win->ops[win->op_count++] = strdup(op_json);
}

static bool parse_hex_color(const char* hex, double* r, double* g, double* b) {
    if (hex[0] == '#') hex++;
    size_t len = strlen(hex);
    if (len == 6) {
        unsigned int ri, gi, bi;
        if (sscanf(hex, "%02x%02x%02x", &ri, &gi, &bi) == 3) {
            *r = ri / 255.0;
            *g = gi / 255.0;
            *b = bi / 255.0;
            return true;
        }
    } else if (len == 3) {
        unsigned int ri, gi, bi;
        if (sscanf(hex, "%1x%1x%1x", &ri, &gi, &bi) == 3) {
            *r = (ri * 17) / 255.0;
            *g = (gi * 17) / 255.0;
            *b = (bi * 17) / 255.0;
            return true;
        }
    }
    return false;
}

static bool parse_color_from_value(Value v, double* r, double* g, double* b) {
    if (is_string(v)) {
        const char* s = as_string(v)->chars;
        if (s[0] == '#') {
            return parse_hex_color(s, r, g, b);
        }
        if (strcasecmp(s, "red") == 0 || strcasecmp(s, "kirmizi") == 0) {
            *r = 1.0; *g = 0.0; *b = 0.0; return true;
        }
        if (strcasecmp(s, "green") == 0 || strcasecmp(s, "yesil") == 0) {
            *r = 0.0; *g = 0.8; *b = 0.0; return true;
        }
        if (strcasecmp(s, "blue") == 0 || strcasecmp(s, "mavi") == 0) {
            *r = 0.1; *g = 0.4; *b = 1.0; return true;
        }
        if (strcasecmp(s, "white") == 0 || strcasecmp(s, "beyaz") == 0) {
            *r = 1.0; *g = 1.0; *b = 1.0; return true;
        }
        if (strcasecmp(s, "black") == 0 || strcasecmp(s, "siyah") == 0) {
            *r = 0.0; *g = 0.0; *b = 0.0; return true;
        }
        if (strcasecmp(s, "yellow") == 0 || strcasecmp(s, "sari") == 0) {
            *r = 1.0; *g = 0.85; *b = 0.0; return true;
        }
        if (strcasecmp(s, "cyan") == 0 || strcasecmp(s, "turkuaz") == 0) {
            *r = 0.0; *g = 0.9; *b = 1.0; return true;
        }
        if (strcasecmp(s, "magenta") == 0 || strcasecmp(s, "mor") == 0 || strcasecmp(s, "purple") == 0) {
            *r = 0.7; *g = 0.1; *b = 0.9; return true;
        }
        if (strcasecmp(s, "orange") == 0 || strcasecmp(s, "turuncu") == 0) {
            *r = 1.0; *g = 0.55; *b = 0.0; return true;
        }
        if (strcasecmp(s, "pink") == 0 || strcasecmp(s, "pembe") == 0) {
            *r = 1.0; *g = 0.4; *b = 0.7; return true;
        }
        if (strcasecmp(s, "gray") == 0 || strcasecmp(s, "grey") == 0 || strcasecmp(s, "gri") == 0) {
            *r = 0.5; *g = 0.5; *b = 0.5; return true;
        }
        if (strcasecmp(s, "darkgray") == 0 || strcasecmp(s, "koyugri") == 0) {
            *r = 0.2; *g = 0.2; *b = 0.2; return true;
        }
        if (strcasecmp(s, "lightgray") == 0 || strcasecmp(s, "acikgri") == 0) {
            *r = 0.8; *g = 0.8; *b = 0.8; return true;
        }
        return parse_hex_color(s, r, g, b);
    }
    if (is_list(v)) {
        ObjList* l = as_list(v);
        if (l->count >= 3 && is_number(l->items[0]) && is_number(l->items[1]) && is_number(l->items[2])) {
            double n0 = as_number(l->items[0]);
            double n1 = as_number(l->items[1]);
            double n2 = as_number(l->items[2]);
            *r = n0 > 1.0 ? n0 / 255.0 : n0;
            *g = n1 > 1.0 ? n1 / 255.0 : n1;
            *b = n2 > 1.0 ? n2 / 255.0 : n2;
            return true;
        }
    }
    if (is_number(v)) {
        double n = as_number(v);
        double val = n > 1.0 ? n / 255.0 : n;
        *r = val; *g = val; *b = val;
        return true;
    }
    return false;
}

static void parse_color_args(size_t argc, Value* args, size_t start_idx, double* r, double* g, double* b,
                             double def_r, double def_g, double def_b) {
    *r = def_r; *g = def_g; *b = def_b;
    if (start_idx >= argc) return;

    if (argc - start_idx >= 3 && is_number(args[start_idx]) && is_number(args[start_idx + 1]) && is_number(args[start_idx + 2])) {
        double n0 = as_number(args[start_idx]);
        double n1 = as_number(args[start_idx + 1]);
        double n2 = as_number(args[start_idx + 2]);
        *r = n0 > 1.0 ? n0 / 255.0 : n0;
        *g = n1 > 1.0 ? n1 / 255.0 : n1;
        *b = n2 > 1.0 ? n2 / 255.0 : n2;
        return;
    }

    parse_color_from_value(args[start_idx], r, g, b);
}

#ifndef NO_X11
static Display* open_display_fallback(void) {
    Display* d = XOpenDisplay(NULL);
    if (d) return d;

    // Oturum Xwayland yetkilendirme dosyasını bul (GNOME Mutter)
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "/run/user/%d/.mutter-Xwaylandauth.*", getuid());
    glob_t g;
    if (glob(pattern, 0, NULL, &g) == 0 && g.gl_pathc > 0) {
        char auth[512];
        snprintf(auth, sizeof(auth), "%s", g.gl_pathv[0]);
        globfree(&g);
        setenv("XAUTHORITY", auth, 1);
        setenv("DISPLAY", ":0", 1);
        d = XOpenDisplay(NULL);
        if (d) return d;
    }

    static const char* try_displays[] = {":0", ":1", ":2"};
    for (size_t i = 0; i < sizeof(try_displays) / sizeof(try_displays[0]) && !d; i++) {
        setenv("DISPLAY", try_displays[i], 1);
        d = XOpenDisplay(NULL);
    }
    return d;
}
#endif

GuiWindow* new_gui_window(int width, int height, const char* title) {
    bool is_headless = gui_headless_mode();
#ifndef NO_X11
    Display* d = NULL;

    if (!is_headless) {
        d = open_display_fallback();
        if (!d) {
            fprintf(stderr, "[CicekliScript GUI]: Grafik sunucusuna (X11/Wayland) bağlanılamadı, web/sanal (headless) modda çalıştırılıyor.\n");
            is_headless = true;
        }
    }
#endif

    if (is_headless) {
        GuiWindow* win = (GuiWindow*)gc_allocate(sizeof(GuiWindow), OBJ_WINDOW);
#ifdef _WIN32
        win->hwnd = NULL;
        win->hdc_mem = NULL;
        win->hbm_mem = NULL;
        win->hbm_old = NULL;
        win->hfont = NULL;
#endif
        win->display = NULL;
        win->screen = 0;
        win->xwindow = 0;
        win->pixmap = 0;
        win->gc = NULL;
        win->wm_delete = 0;
        win->surface = NULL;
        win->cr = NULL;
        win->width = width;
        win->height = height;
        win->title = strdup(title ? title : "CicekliScript Penceresi");
        win->is_open = true;
        win->bg_r = 1.0;
        win->bg_g = 1.0;
        win->bg_b = 1.0;
        win->mouse_x = 0;
        win->mouse_y = 0;
        win->mouse_down = false;
        win->last_key[0] = '\0';
        win->headless = true;
        win->emitted = false;
        win->op_count = 0;
        win->op_cap = 0;
        win->ops = NULL;
        return win;
    }

#ifdef _WIN32
    if (!win32_class_registered) {
        WNDCLASSA wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = CicekliWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "CicekliScriptWindow";
        RegisterClassA(&wc);
        win32_class_registered = true;
    }

    RECT rc = {0, 0, width, height};
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    int win_w = rc.right - rc.left;
    int win_h = rc.bottom - rc.top;

    HWND hwnd = CreateWindowExA(
        0,
        "CicekliScriptWindow",
        title ? title : "CicekliScript Penceresi",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_w, win_h,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hwnd) {
        fprintf(stderr, "[CicekliScript GUI]: Windows penceresi oluşturulamadı!\n");
        return NULL;
    }

    HDC screen_dc = GetDC(hwnd);
    HDC mem_dc = CreateCompatibleDC(screen_dc);
    HBITMAP mem_bm = CreateCompatibleBitmap(screen_dc, width, height);
    HBITMAP old_bm = (HBITMAP)SelectObject(mem_dc, mem_bm);
    ReleaseDC(hwnd, screen_dc);

    GuiWindow* win = (GuiWindow*)gc_allocate(sizeof(GuiWindow), OBJ_WINDOW);
    win->hwnd = hwnd;
    win->hdc_mem = mem_dc;
    win->hbm_mem = mem_bm;
    win->hbm_old = old_bm;
    win->hfont = NULL;
    win->display = NULL;
    win->screen = 0;
    win->xwindow = 0;
    win->pixmap = 0;
    win->gc = NULL;
    win->wm_delete = 0;
    win->surface = NULL;
    win->cr = NULL;
    win->width = width;
    win->height = height;
    win->title = strdup(title ? title : "CicekliScript Penceresi");
    win->is_open = true;
    win->bg_r = 1.0;
    win->bg_g = 1.0;
    win->bg_b = 1.0;
    win->mouse_x = 0;
    win->mouse_y = 0;
    win->mouse_down = false;
    win->last_key[0] = '\0';
    win->headless = false;
    win->emitted = false;
    win->op_count = 0;
    win->op_cap = 0;
    win->ops = NULL;

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)win);

    // Başlangıçta beyaz arka planla doldur
    HBRUSH br = CreateSolidBrush(RGB(255, 255, 255));
    RECT client_rc = {0, 0, width, height};
    FillRect(mem_dc, &client_rc, br);
    DeleteObject(br);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return win;
#elif !defined(NO_X11)
    int screen = DefaultScreen(d);
    Window root = RootWindow(d, screen);

    XSetWindowAttributes attrs;
    attrs.background_pixel = WhitePixel(d, screen);
    attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask |
                       PointerMotionMask | StructureNotifyMask;

    Window w = XCreateWindow(d, root, 100, 100, width, height, 0,
                             DefaultDepth(d, screen), InputOutput,
                             DefaultVisual(d, screen),
                             CWBackPixel | CWEventMask, &attrs);

    XStoreName(d, w, title ? title : "CicekliScript Penceresi");

    // Pencere yöneticisi boyut ve durum ipuçları
    XSizeHints size_hints;
    size_hints.flags = PSize | PMinSize;
    size_hints.width = width;
    size_hints.height = height;
    size_hints.min_width = width;
    size_hints.min_height = height;
    XSetWMNormalHints(d, w, &size_hints);

    XWMHints wm_hints;
    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;
    XSetWMHints(d, w, &wm_hints);

    Atom wm_delete = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, w, &wm_delete, 1);

    XMapWindow(d, w);
    XFlush(d);

    // Çift arabellek için sunucu tarafı Pixmap ve GC oluştur
    Pixmap pixmap = XCreatePixmap(d, w, (unsigned int)width, (unsigned int)height, (unsigned int)DefaultDepth(d, screen));
    GC gc = XCreateGC(d, w, 0, NULL);

    cairo_surface_t* surface = cairo_xlib_surface_create(d, pixmap, DefaultVisual(d, screen), width, height);
    cairo_t* cr = cairo_create(surface);

    GuiWindow* win = (GuiWindow*)gc_allocate(sizeof(GuiWindow), OBJ_WINDOW);
    win->display = d;
    win->screen = screen;
    win->xwindow = w;
    win->pixmap = pixmap;
    win->gc = gc;
    win->wm_delete = wm_delete;
    win->surface = surface;
    win->cr = cr;
    win->width = width;
    win->height = height;
    win->title = strdup(title ? title : "CicekliScript Penceresi");
    win->is_open = true;
    win->bg_r = 1.0;
    win->bg_g = 1.0;
    win->bg_b = 1.0;
    win->mouse_x = 0;
    win->mouse_y = 0;
    win->mouse_down = false;
    win->last_key[0] = '\0';
    win->headless = false;
    win->emitted = false;
    win->op_count = 0;
    win->op_cap = 0;
    win->ops = NULL;

    // Başlangıçta beyaz arka planla doldur
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);
    gui_window_update(win);

    return win;
#else
    return NULL;
#endif
}

void gui_cleanup(void) {
#ifndef NO_X11
    cairo_debug_reset_static_data();
#endif
}

static void emit_headless_json(GuiWindow* win) {
    if (!win || !win->ops || win->op_count == 0 || win->emitted) return;
    win->emitted = true;
    char title_e[512];
    json_escape(win->title ? win->title : "", title_e, sizeof(title_e));
    printf("CICEKLI_GUI {\"w\":%d,\"h\":%d,\"title\":\"%s\",\"ops\":[", win->width, win->height, title_e);
    for (int i = 0; i < win->op_count; i++) {
        printf("%s%s", i > 0 ? "," : "", win->ops[i]);
    }
    printf("]}\n");
    fflush(stdout);
}

void gui_window_destroy(GuiWindow* win) {
    if (!win) return;
    if (win->headless) {
        emit_headless_json(win);
        for (int i = 0; i < win->op_count; i++) free(win->ops[i]);
        free(win->ops);
        win->ops = NULL;
        win->op_count = 0;
        if (win->title) {
            free(win->title);
            win->title = NULL;
        }
        return;
    }

#ifdef _WIN32
    if (win->hdc_mem) {
        if (win->hbm_old) {
            SelectObject((HDC)win->hdc_mem, (HBITMAP)win->hbm_old);
            win->hbm_old = NULL;
        }
        if (win->hbm_mem) {
            DeleteObject((HBITMAP)win->hbm_mem);
            win->hbm_mem = NULL;
        }
        DeleteDC((HDC)win->hdc_mem);
        win->hdc_mem = NULL;
    }
    if (win->hwnd) {
        DestroyWindow((HWND)win->hwnd);
        win->hwnd = NULL;
    }
#elif !defined(NO_X11)
    if (win->cr) {
        cairo_destroy(win->cr);
        win->cr = NULL;
    }
    if (win->surface) {
        cairo_surface_destroy(win->surface);
        win->surface = NULL;
    }
    if (win->display) {
        if (win->gc) {
            XFreeGC(win->display, win->gc);
            win->gc = NULL;
        }
        if (win->pixmap) {
            XFreePixmap(win->display, win->pixmap);
            win->pixmap = 0;
        }
        if (win->is_open && win->xwindow) {
            XDestroyWindow(win->display, win->xwindow);
            win->xwindow = 0;
        }
        XCloseDisplay(win->display);
        win->display = NULL;
    }
#endif
    win->is_open = false;
    if (win->ops) {
        for (int i = 0; i < win->op_count; i++) free(win->ops[i]);
        free(win->ops);
        win->ops = NULL;
    }
    if (win->title) {
        free(win->title);
        win->title = NULL;
    }
}

void gui_window_poll_events(GuiWindow* win) {
    if (!win || win->headless || !win->is_open) return;

#ifdef _WIN32
    if (win->hwnd) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
#elif !defined(NO_X11)
    if (!win->display) return;

    while (XPending(win->display) > 0) {
        XEvent ev;
        XNextEvent(win->display, &ev);
        switch (ev.type) {
            case ClientMessage:
                if ((Atom)ev.xclient.data.l[0] == win->wm_delete) {
                    win->is_open = false;
                }
                break;
            case DestroyNotify:
                win->is_open = false;
                break;
            case Expose:
                // Pixmap'ten ekrana kopyala
                if (win->pixmap && win->xwindow && win->gc) {
                    XCopyArea(win->display, win->pixmap, win->xwindow, win->gc, 0, 0,
                              (unsigned int)win->width, (unsigned int)win->height, 0, 0);
                    XFlush(win->display);
                }
                break;
            case ConfigureNotify:
                win->width = ev.xconfigure.width;
                win->height = ev.xconfigure.height;
                break;
            case MotionNotify:
                win->mouse_x = ev.xmotion.x;
                win->mouse_y = ev.xmotion.y;
                break;
            case ButtonPress:
                if (ev.xbutton.button == Button1) {
                    win->mouse_down = true;
                    win->mouse_x = ev.xbutton.x;
                    win->mouse_y = ev.xbutton.y;
                }
                break;
            case ButtonRelease:
                if (ev.xbutton.button == Button1) {
                    win->mouse_down = false;
                }
                break;
            case KeyPress: {
                char kbuf[32];
                KeySym ks;
                int len = XLookupString(&ev.xkey, kbuf, sizeof(kbuf) - 1, &ks, NULL);
                if (len > 0) {
                    if ((size_t)len >= sizeof(win->last_key)) len = sizeof(win->last_key) - 1;
                    memcpy(win->last_key, kbuf, len);
                    win->last_key[len] = '\0';
                }
                if (ks == XK_Escape) {
                    win->is_open = false;
                }
                break;
            }
        }
    }
#endif
}

void gui_window_update(GuiWindow* win) {
    if (!win || !win->is_open) return;

    if (win->headless) {
        return;
    }

#ifdef _WIN32
    if (win->hwnd && win->hdc_mem) {
        HDC hdc = GetDC((HWND)win->hwnd);
        BitBlt(hdc, 0, 0, win->width, win->height, (HDC)win->hdc_mem, 0, 0, SRCCOPY);
        ReleaseDC((HWND)win->hwnd, hdc);
        gui_window_poll_events(win);
    }
#elif !defined(NO_X11)
    if (win->display && win->xwindow && win->pixmap && win->gc) {
        cairo_surface_flush(win->surface);
        XCopyArea(win->display, win->pixmap, win->xwindow, win->gc, 0, 0,
                  (unsigned int)win->width, (unsigned int)win->height, 0, 0);
        XFlush(win->display);
        gui_window_poll_events(win);
    }
#endif
}

void gui_window_color(GuiWindow* win, double r, double g, double b) {
    if (!win) return;
    win->bg_r = r;
    win->bg_g = g;
    win->bg_b = b;

    char buf[128];
    snprintf(buf, sizeof(buf), "[\"color\",%.3f,%.3f,%.3f]", r, g, b);
    gui_record_op(win, buf);

    if (win->headless) return;

#ifdef _WIN32
    if (win->hdc_mem) {
        HBRUSH br = CreateSolidBrush(RGB((int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0)));
        RECT rc = {0, 0, win->width, win->height};
        FillRect((HDC)win->hdc_mem, &rc, br);
        DeleteObject(br);
        gui_window_update(win);
    }
#elif !defined(NO_X11)
    if (!win->cr) return;
    cairo_set_source_rgb(win->cr, r, g, b);
    cairo_paint(win->cr);
    gui_window_update(win);
#endif
}

void gui_window_text(GuiWindow* win, const char* text, double r, double g, double b, double size, double x, double y) {
    if (!win || !text) return;

    char et[2048];
    char buf[2304];
    json_escape(text, et, sizeof(et));
    snprintf(buf, sizeof(buf), "[\"text\",\"%s\",%.3f,%.3f,%.3f,%.1f,%.1f,%.1f]", et, r, g, b,
             size > 0 ? size : 14.0, x, y);
    gui_record_op(win, buf);

    if (win->headless) return;

#ifdef _WIN32
    if (win->hdc_mem) {
        HDC hdc = (HDC)win->hdc_mem;
        COLORREF col = RGB((int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
        SetTextColor(hdc, col);
        SetBkMode(hdc, TRANSPARENT);
        int font_h = size > 0 ? (int)size : 14;
        HFONT hf = CreateFontA(font_h, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HFONT old_hf = (HFONT)SelectObject(hdc, hf);
        TextOutA(hdc, (int)x, (int)y, text, (int)strlen(text));
        SelectObject(hdc, old_hf);
        DeleteObject(hf);
        gui_window_update(win);
    }
#elif !defined(NO_X11)
    if (!win->cr) return;
    cairo_select_font_face(win->cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(win->cr, size > 0 ? size : 14.0);
    cairo_set_source_rgb(win->cr, r, g, b);
    cairo_move_to(win->cr, x, y + size);
    cairo_show_text(win->cr, text);
    gui_window_update(win);
#endif
}

void gui_window_rect(GuiWindow* win, double x, double y, double w, double h, double r, double g, double b) {
    if (!win) return;

    char buf[128];
    snprintf(buf, sizeof(buf), "[\"rect\",%.1f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f]", x, y, w, h, r, g, b);
    gui_record_op(win, buf);

    if (win->headless) return;

#ifdef _WIN32
    if (win->hdc_mem) {
        HDC hdc = (HDC)win->hdc_mem;
        COLORREF col = RGB((int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
        HBRUSH br = CreateSolidBrush(col);
        HPEN pen = CreatePen(PS_SOLID, 1, col);
        HBRUSH old_br = (HBRUSH)SelectObject(hdc, br);
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);
        Rectangle(hdc, (int)x, (int)y, (int)(x + w), (int)(y + h));
        SelectObject(hdc, old_br);
        SelectObject(hdc, old_pen);
        DeleteObject(br);
        DeleteObject(pen);
        gui_window_update(win);
    }
#elif !defined(NO_X11)
    if (!win->cr) return;
    cairo_set_source_rgb(win->cr, r, g, b);
    cairo_rectangle(win->cr, x, y, w, h);
    cairo_fill(win->cr);
    gui_window_update(win);
#endif
}

void gui_window_circle(GuiWindow* win, double x, double y, double radius, double r, double g, double b) {
    if (!win) return;

    char buf[128];
    snprintf(buf, sizeof(buf), "[\"circle\",%.1f,%.1f,%.1f,%.3f,%.3f,%.3f]", x, y, radius, r, g, b);
    gui_record_op(win, buf);

    if (win->headless) return;

#ifdef _WIN32
    if (win->hdc_mem) {
        HDC hdc = (HDC)win->hdc_mem;
        COLORREF col = RGB((int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
        HBRUSH br = CreateSolidBrush(col);
        HPEN pen = CreatePen(PS_SOLID, 1, col);
        HBRUSH old_br = (HBRUSH)SelectObject(hdc, br);
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);
        Ellipse(hdc, (int)(x - radius), (int)(y - radius), (int)(x + radius), (int)(y + radius));
        SelectObject(hdc, old_br);
        SelectObject(hdc, old_pen);
        DeleteObject(br);
        DeleteObject(pen);
        gui_window_update(win);
    }
#elif !defined(NO_X11)
    if (!win->cr) return;
    cairo_set_source_rgb(win->cr, r, g, b);
    cairo_arc(win->cr, x, y, radius, 0, 2 * M_PI);
    cairo_fill(win->cr);
    gui_window_update(win);
#endif
}

void gui_window_line(GuiWindow* win, double x1, double y1, double x2, double y2, double r, double g, double b, double width) {
    if (!win) return;

    char buf[160];
    snprintf(buf, sizeof(buf), "[\"line\",%.1f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f,%.1f]", x1, y1, x2, y2, r, g, b,
             width > 0 ? width : 1.0);
    gui_record_op(win, buf);

    if (win->headless) return;

#ifdef _WIN32
    if (win->hdc_mem) {
        HDC hdc = (HDC)win->hdc_mem;
        COLORREF col = RGB((int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
        int pen_w = width > 1.0 ? (int)width : 1;
        HPEN pen = CreatePen(PS_SOLID, pen_w, col);
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, (int)x1, (int)y1, NULL);
        LineTo(hdc, (int)x2, (int)y2);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
        gui_window_update(win);
    }
#elif !defined(NO_X11)
    if (!win->cr) return;
    cairo_set_source_rgb(win->cr, r, g, b);
    cairo_set_line_width(win->cr, width > 0 ? width : 1.0);
    cairo_move_to(win->cr, x1, y1);
    cairo_line_to(win->cr, x2, y2);
    cairo_stroke(win->cr);
    gui_window_update(win);
#endif
}

Value native_mete_window(Value self, size_t argc, Value* args, int line) {
    (void)self;
    int width = 400;
    int height = 300;
    const char* title = "CicekliScript Penceresi";

    if (argc >= 2) {
        if (!is_number(args[0]) || !is_number(args[1])) {
            fprintf(stderr, "CicekliScript Hatası (Satır %d): cicekli.window(genislik, yukseklik) sayı bekler!\n", line);
            return make_null();
        }
        width = (int)as_number(args[0]);
        height = (int)as_number(args[1]);
    }
    if (argc >= 3 && is_string(args[2])) {
        title = as_string(args[2])->chars;
    }

    GuiWindow* win = new_gui_window(width, height, title);
    if (!win) {
        return make_null();
    }
    return make_obj((Obj*)win);
}

static Value native_win_color(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;

    double r = 1.0, g = 1.0, b = 1.0;
    if (argc == 0) {
        r = 1.0; g = 1.0; b = 1.0;
    } else {
        parse_color_args(argc, args, 0, &r, &g, &b, win->bg_r, win->bg_g, win->bg_b);
    }
    gui_window_color(win, r, g, b);
    return self;
}

static Value native_win_text(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;

    if (argc < 1) return self;

    char* text_str = value_to_string(args[0]);
    double r = 0.0, g = 0.0, b = 0.0;
    double size = 16.0;
    double posx = 10.0;
    double posy = 10.0;

    if (argc >= 7 && is_number(args[1]) && is_number(args[2]) && is_number(args[3])) {
        // window.text(metin, r, g, b, size, posx, posy)
        double nr = as_number(args[1]);
        double ng = as_number(args[2]);
        double nb = as_number(args[3]);
        r = nr > 1.0 ? nr / 255.0 : nr;
        g = ng > 1.0 ? ng / 255.0 : ng;
        b = nb > 1.0 ? nb / 255.0 : nb;
        if (is_number(args[4])) size = as_number(args[4]);
        if (argc >= 6 && is_number(args[5])) posx = as_number(args[5]);
        if (argc >= 7 && is_number(args[6])) posy = as_number(args[6]);
    } else {
        // window.text(metin, color, size, posx, posy)
        if (argc >= 2) {
            parse_color_from_value(args[1], &r, &g, &b);
        }
        if (argc >= 3 && is_number(args[2])) {
            size = as_number(args[2]);
        }
        if (argc >= 4 && is_number(args[3])) {
            posx = as_number(args[3]);
        }
        if (argc >= 5 && is_number(args[4])) {
            posy = as_number(args[4]);
        }
    }

    gui_window_text(win, text_str, r, g, b, size, posx, posy);
    free(text_str);
    return self;
}

static Value native_win_rect(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;

    if (argc < 4) return self;
    double x = as_number(args[0]);
    double y = as_number(args[1]);
    double w = as_number(args[2]);
    double h = as_number(args[3]);

    double r = 0.0, g = 0.0, b = 0.0;
    if (argc >= 5) {
        parse_color_args(argc, args, 4, &r, &g, &b, 0.0, 0.0, 0.0);
    }

    gui_window_rect(win, x, y, w, h, r, g, b);
    return self;
}

static Value native_win_circle(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;

    if (argc < 3) return self;
    double x = as_number(args[0]);
    double y = as_number(args[1]);
    double radius = as_number(args[2]);

    double r = 0.0, g = 0.0, b = 0.0;
    if (argc >= 4) {
        parse_color_args(argc, args, 3, &r, &g, &b, 0.0, 0.0, 0.0);
    }

    gui_window_circle(win, x, y, radius, r, g, b);
    return self;
}

static Value native_win_line(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;

    if (argc < 4) return self;
    double x1 = as_number(args[0]);
    double y1 = as_number(args[1]);
    double x2 = as_number(args[2]);
    double y2 = as_number(args[3]);

    double r = 0.0, g = 0.0, b = 0.0;
    double width = 1.0;
    if (argc >= 5) {
        parse_color_args(argc, args, 4, &r, &g, &b, 0.0, 0.0, 0.0);
    }
    if (argc >= 6 && is_number(args[5])) {
        width = as_number(args[5]);
    }

    gui_window_line(win, x1, y1, x2, y2, r, g, b, width);
    return self;
}

static Value native_win_update(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;
    gui_window_update(win);
    return self;
}

static Value native_win_is_open(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_window(self)) return make_bool(false);
    GuiWindow* win = (GuiWindow*)self.as.obj;
    gui_window_poll_events(win);
    return make_bool(win->is_open);
}

static Value native_win_close(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;
    gui_window_destroy(win);
    return make_null();
}

static Value native_win_sleep(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;
    int ms = 16;
    if (argc >= 1 && is_number(args[0])) {
        ms = (int)as_number(args[0]);
    }
    gui_window_update(win);
    if (win->headless) return self;
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep(ms * 1000);
#endif
    gui_window_poll_events(win);
    return self;
}

static Value native_win_mainloop(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_window(self)) return make_null();
    GuiWindow* win = (GuiWindow*)self.as.obj;

    gui_window_update(win);

    if (win->headless) {
        emit_headless_json(win);
        return make_null();
    }

    while (win->is_open) {
        gui_window_poll_events(win);
        if (!win->is_open) break;

        if (argc >= 1 && (is_function(args[0]) || is_native(args[0]))) {
            // Callback desteği
        }

        gui_window_update(win);
#ifdef _WIN32
        Sleep(16); // ~60 FPS
#else
        usleep(16000); // ~60 FPS
#endif
    }

    return make_null();
}

static Value native_win_get_mouse_x(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_window(self)) return make_number(0);
    GuiWindow* win = (GuiWindow*)self.as.obj;
    gui_window_poll_events(win);
    return make_number(win->mouse_x);
}

static Value native_win_get_mouse_y(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_window(self)) return make_number(0);
    GuiWindow* win = (GuiWindow*)self.as.obj;
    gui_window_poll_events(win);
    return make_number(win->mouse_y);
}

static Value native_win_is_mouse_down(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_window(self)) return make_bool(false);
    GuiWindow* win = (GuiWindow*)self.as.obj;
    gui_window_poll_events(win);
    return make_bool(win->mouse_down);
}

Value gui_window_get_method(Value win_val, const char* name) {
    (void)win_val;
    if (strcmp(name, "color") == 0) return make_obj((Obj*)new_native("color", native_win_color, -1));
    if (strcmp(name, "text") == 0) return make_obj((Obj*)new_native("text", native_win_text, -1));
    if (strcmp(name, "rect") == 0) return make_obj((Obj*)new_native("rect", native_win_rect, -1));
    if (strcmp(name, "circle") == 0) return make_obj((Obj*)new_native("circle", native_win_circle, -1));
    if (strcmp(name, "line") == 0) return make_obj((Obj*)new_native("line", native_win_line, -1));
    if (strcmp(name, "clear") == 0) return make_obj((Obj*)new_native("clear", native_win_color, -1));
    if (strcmp(name, "update") == 0) return make_obj((Obj*)new_native("update", native_win_update, 0));
    if (strcmp(name, "show") == 0) return make_obj((Obj*)new_native("show", native_win_update, 0));
    if (strcmp(name, "is_open") == 0) return make_obj((Obj*)new_native("is_open", native_win_is_open, 0));
    if (strcmp(name, "close") == 0) return make_obj((Obj*)new_native("close", native_win_close, 0));
    if (strcmp(name, "sleep") == 0) return make_obj((Obj*)new_native("sleep", native_win_sleep, -1));
    if (strcmp(name, "mainloop") == 0) return make_obj((Obj*)new_native("mainloop", native_win_mainloop, -1));
    if (strcmp(name, "run") == 0) return make_obj((Obj*)new_native("run", native_win_mainloop, -1));
    if (strcmp(name, "get_mouse_x") == 0) return make_obj((Obj*)new_native("get_mouse_x", native_win_get_mouse_x, 0));
    if (strcmp(name, "get_mouse_y") == 0) return make_obj((Obj*)new_native("get_mouse_y", native_win_get_mouse_y, 0));
    if (strcmp(name, "is_mouse_down") == 0) return make_obj((Obj*)new_native("is_mouse_down", native_win_is_mouse_down, 0));
    return make_null();
}

Value gui_window_get_property(Value win_val, const char* name) {
    if (!is_window(win_val)) return make_null();
    GuiWindow* win = (GuiWindow*)win_val.as.obj;
    if (strcmp(name, "width") == 0 || strcmp(name, "genislik") == 0) return make_number(win->width);
    if (strcmp(name, "height") == 0 || strcmp(name, "yukseklik") == 0) return make_number(win->height);
    if (strcmp(name, "mouse_x") == 0) { gui_window_poll_events(win); return make_number(win->mouse_x); }
    if (strcmp(name, "mouse_y") == 0) { gui_window_poll_events(win); return make_number(win->mouse_y); }
    return make_null();
}
