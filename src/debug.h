#ifndef CICEKLISCRIPT_DEBUG_H
#define CICEKLISCRIPT_DEBUG_H

#include <stdbool.h>
#include <stddef.h>
#include "ast.h"
#include "env.h"
#include "value.h"

typedef enum {
    DEBUG_MODE_CONTINUE,
    DEBUG_MODE_STEP_IN,
    DEBUG_MODE_STEP_OVER,
    DEBUG_MODE_STEP_OUT
} DebugStepMode;

typedef struct Breakpoint {
    int id;
    char* file;
    int line;
    bool enabled;
    int hit_count;
    struct Breakpoint* next;
} Breakpoint;

typedef struct CallFrame {
    char* function_name;
    const char* file;
    int line;
    Environment* env;
} CallFrame;

void debugger_init(bool dap_mode);
void debugger_cleanup(void);
void debugger_set_source(const char* filename, const char* source);
void debugger_push_frame(const char* fn_name, const char* file, int line, Environment* env);
void debugger_pop_frame(void);
void debugger_update_frame_line(int line);
void debugger_on_statement(Stmt* stmt, Environment* env);

bool debugger_is_active(void);
bool debugger_is_dap(void);
void debugger_set_stop_on_entry(bool stop);

int debugger_add_breakpoint(const char* file, int line);
bool debugger_remove_breakpoint(int id);
void debugger_list_breakpoints(void);
void debugger_clear_breakpoints(void);

// DAP protocol handler
int debugger_run_dap(void);

#endif // CICEKLISCRIPT_DEBUG_H
