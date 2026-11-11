#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "debug.h"
#include "eval.h"
#include "parser.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define MAX_FRAMES 256

typedef struct {
    bool active;
    bool is_dap;
    bool stop_on_entry;
    bool has_stopped_entry;
    DebugStepMode step_mode;
    int step_depth;
    int last_stop_line;
    const char* current_file;
    char* source_code;
    char** source_lines;
    size_t line_count;
    Breakpoint* breakpoints;
    int next_bp_id;
    CallFrame stack[MAX_FRAMES];
    int stack_depth;
    int dap_seq;
} DebuggerState;

static DebuggerState dbg = {
    .active = false,
    .is_dap = false,
    .stop_on_entry = false,
    .has_stopped_entry = false,
    .step_mode = DEBUG_MODE_CONTINUE,
    .step_depth = 0,
    .last_stop_line = -1,
    .current_file = "main.ccl",
    .source_code = NULL,
    .source_lines = NULL,
    .line_count = 0,
    .breakpoints = NULL,
    .next_bp_id = 1,
    .stack_depth = 0,
    .dap_seq = 1
};

void debugger_init(bool dap_mode) {
    dbg.active = true;
    dbg.is_dap = dap_mode;
    dbg.stop_on_entry = true;
    dbg.has_stopped_entry = false;
    dbg.step_mode = dap_mode ? DEBUG_MODE_CONTINUE : DEBUG_MODE_STEP_IN;
    dbg.step_depth = 0;
    dbg.last_stop_line = -1;
    dbg.stack_depth = 0;
    dbg.dap_seq = 1;
}

void debugger_cleanup(void) {
    debugger_clear_breakpoints();
    if (dbg.source_lines) {
        free(dbg.source_lines);
        dbg.source_lines = NULL;
    }
    if (dbg.source_code) {
        free(dbg.source_code);
        dbg.source_code = NULL;
    }
    for (int i = 0; i < dbg.stack_depth; i++) {
        if (dbg.stack[i].function_name) {
            free(dbg.stack[i].function_name);
            dbg.stack[i].function_name = NULL;
        }
    }
    dbg.stack_depth = 0;
    dbg.active = false;
}

bool debugger_is_active(void) {
    return dbg.active;
}

bool debugger_is_dap(void) {
    return dbg.active && dbg.is_dap;
}

void debugger_set_stop_on_entry(bool stop) {
    dbg.stop_on_entry = stop;
}

void debugger_set_source(const char* filename, const char* source) {
    if (filename) dbg.current_file = filename;
    if (dbg.source_code) free(dbg.source_code);
    if (dbg.source_lines) free(dbg.source_lines);

    if (!source) {
        dbg.source_code = NULL;
        dbg.source_lines = NULL;
        dbg.line_count = 0;
        return;
    }

    dbg.source_code = strdup(source);
    size_t count = 1;
    for (const char* p = source; *p; p++) {
        if (*p == '\n') count++;
    }

    dbg.source_lines = (char**)malloc(sizeof(char*) * (count + 1));
    dbg.line_count = 0;

    char* line = dbg.source_code;
    for (char* p = dbg.source_code; *p; p++) {
        if (*p == '\n') {
            *p = '\0';
            if (p > line && *(p - 1) == '\r') *(p - 1) = '\0';
            dbg.source_lines[dbg.line_count++] = line;
            line = p + 1;
        }
    }
    if (*line != '\0' || dbg.line_count == 0) {
        dbg.source_lines[dbg.line_count++] = line;
    }
}

int debugger_add_breakpoint(const char* file, int line) {
    Breakpoint* bp = (Breakpoint*)malloc(sizeof(Breakpoint));
    bp->id = dbg.next_bp_id++;
    bp->file = file ? strdup(file) : NULL;
    bp->line = line;
    bp->enabled = true;
    bp->hit_count = 0;
    bp->next = dbg.breakpoints;
    dbg.breakpoints = bp;
    return bp->id;
}

bool debugger_remove_breakpoint(int id) {
    Breakpoint** curr = &dbg.breakpoints;
    while (*curr) {
        if ((*curr)->id == id) {
            Breakpoint* to_del = *curr;
            *curr = to_del->next;
            if (to_del->file) free(to_del->file);
            free(to_del);
            return true;
        }
        curr = &(*curr)->next;
    }
    return false;
}

void debugger_clear_breakpoints(void) {
    Breakpoint* curr = dbg.breakpoints;
    while (curr) {
        Breakpoint* next = curr->next;
        if (curr->file) free(curr->file);
        free(curr);
        curr = next;
    }
    dbg.breakpoints = NULL;
}

void debugger_list_breakpoints(void) {
    if (!dbg.breakpoints) {
        printf("No breakpoints set.\n");
        return;
    }
    printf("Num   Type          Disp Enb Address/Line  File\n");
    for (Breakpoint* b = dbg.breakpoints; b; b = b->next) {
        printf("%-5d breakpoint    keep %-3s line %-8d %s (hits: %d)\n",
               b->id, b->enabled ? "y" : "n", b->line,
               b->file ? b->file : (dbg.current_file ? dbg.current_file : "<all>"),
               b->hit_count);
    }
}

static Breakpoint* find_breakpoint(const char* file, int line) {
    for (Breakpoint* b = dbg.breakpoints; b; b = b->next) {
        if (!b->enabled) continue;
        if (b->line == line) {
            if (!b->file || !file) return b;
            const char* b_base = strrchr(b->file, '/');
            const char* b_bs = strrchr(b->file, '\\');
            if (b_bs && (!b_base || b_bs > b_base)) b_base = b_bs;
            b_base = b_base ? b_base + 1 : b->file;

            const char* f_base = strrchr(file, '/');
            const char* f_bs = strrchr(file, '\\');
            if (f_bs && (!f_base || f_bs > f_base)) f_base = f_bs;
            f_base = f_base ? f_base + 1 : file;

#if defined(_WIN32)
            if (strcasecmp(b_base, f_base) == 0) return b;
#else
            if (strcmp(b_base, f_base) == 0) return b;
#endif
        }
    }
    return NULL;
}

void debugger_push_frame(const char* fn_name, const char* file, int line, Environment* env) {
    if (dbg.stack_depth < MAX_FRAMES) {
        dbg.stack[dbg.stack_depth].function_name = fn_name ? strdup(fn_name) : strdup("<script>");
        dbg.stack[dbg.stack_depth].file = file ? file : dbg.current_file;
        dbg.stack[dbg.stack_depth].line = line;
        dbg.stack[dbg.stack_depth].env = env;
        dbg.stack_depth++;
    }
}

void debugger_pop_frame(void) {
    if (dbg.stack_depth > 0) {
        dbg.stack_depth--;
        if (dbg.stack[dbg.stack_depth].function_name) {
            free(dbg.stack[dbg.stack_depth].function_name);
            dbg.stack[dbg.stack_depth].function_name = NULL;
        }
    }
}

void debugger_update_frame_line(int line) {
    if (dbg.stack_depth > 0) {
        dbg.stack[dbg.stack_depth - 1].line = line;
    }
}

static void print_source_snippet(int current_line, int window_size) {
    if (!dbg.source_lines || dbg.line_count == 0) return;
    int start = current_line - window_size;
    if (start < 1) start = 1;
    int end = current_line + window_size;
    if ((size_t)end > dbg.line_count) end = (int)dbg.line_count;

    for (int l = start; l <= end; l++) {
        const char* mark = (l == current_line) ? "=>" : "  ";
        printf("%s %4d | %s\n", mark, l, dbg.source_lines[l - 1]);
    }
}

static void print_locals(Environment* env) {
    if (!env) {
        printf("  (no local variables)\n");
        return;
    }
    size_t shown = 0;
    Environment* curr = env;
    while (curr) {
        bool is_global = (curr->enclosing == NULL);
        for (size_t i = 0; i < curr->count; i++) {
            const char* name = curr->entries[i].key;
            Value val = curr->entries[i].value;
            if (is_global && is_native(val)) continue;
            if (is_global && (strcmp(name, "cicekli") == 0 || strcmp(name, "mete") == 0 ||
                              strcmp(name, "ccl") == 0 || strcmp(name, "cicekliscript") == 0 ||
                              strcmp(name, "args") == 0 ||
                              strcmp(name, "math") == 0 || strcmp(name, "file") == 0 ||
                              strcmp(name, "doc") == 0 || strcmp(name, "cuda") == 0)) continue;
            char* str = value_to_string(val);
            printf("  %-16s = %s  [%s]\n", name, str ? str : "null", value_type_name(val));
            if (str) free(str);
            shown++;
        }
        if (is_global) break;
        curr = curr->enclosing;
    }
    if (shown == 0) {
        printf("  (no user variables in scope)\n");
    }
}

static void print_backtrace(int current_line) {
    printf("Call stack backtrace:\n");
    int frame_idx = 0;
    for (int i = dbg.stack_depth - 1; i >= 0; i--) {
        int l = (i == dbg.stack_depth - 1) ? current_line : dbg.stack[i].line;
        printf("  #%-2d %s() at %s:%d\n", frame_idx++,
               dbg.stack[i].function_name ? dbg.stack[i].function_name : "<anonymous>",
               dbg.stack[i].file ? dbg.stack[i].file : "unknown",
               l);
    }
}

static void print_help(void) {
    printf("\nCicekliScript Debugger Commands:\n");
    printf("  s, step         Step into the next statement\n");
    printf("  n, next         Step over to the next statement in the same frame\n");
    printf("  out, finish     Step out of the current function\n");
    printf("  c, continue     Continue program execution until next breakpoint\n");
    printf("  b <line>        Set a breakpoint at line number (e.g. 'b 15')\n");
    printf("  b <file>:<line> Set a breakpoint at file:line (e.g. 'b test.ccl:15')\n");
    printf("  del <id>        Delete breakpoint by ID (e.g. 'del 1')\n");
    printf("  info b          List all active breakpoints\n");
    printf("  p <var|expr>    Print / evaluate variable or expression in current scope\n");
    printf("  locals          List all local variables with values and types\n");
    printf("  bt, backtrace   Show call stack frames\n");
    printf("  l, list [n]     Show source code around current line\n");
    printf("  h, help         Show this help message\n");
    printf("  q, quit         Abort debugging and exit immediately\n\n");
}

/* ========================================================================= */
/* DAP (Debug Adapter Protocol) JSON Communication                           */
/* ========================================================================= */

static void dap_send_raw(const char* json) {
    size_t len = strlen(json);
    printf("Content-Length: %zu\r\n\r\n%s", len, json);
    fflush(stdout);
}

static void dap_json_escape_path(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) return;
    size_t di = 0;
    for (size_t si = 0; src[si] && di + 2 < dst_size; si++) {
        if (src[si] == '\\') {
            dst[di++] = '\\';
            dst[di++] = '\\';
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

void debugger_dap_output(const char* category, const char* text) {
    if (!dbg.active || !dbg.is_dap || !text) return;
    char esc[4096];
    size_t ei = 0;
    for (size_t i = 0; text[i] && ei + 4 < sizeof(esc); i++) {
        if (text[i] == '"') { esc[ei++] = '\\'; esc[ei++] = '"'; }
        else if (text[i] == '\\') { esc[ei++] = '\\'; esc[ei++] = '\\'; }
        else if (text[i] == '\n') { esc[ei++] = '\\'; esc[ei++] = 'n'; }
        else if (text[i] == '\r') { esc[ei++] = '\\'; esc[ei++] = 'r'; }
        else if (text[i] == '\t') { esc[ei++] = '\\'; esc[ei++] = 't'; }
        else { esc[ei++] = text[i]; }
    }
    esc[ei] = '\0';

    char buf[8192];
    snprintf(buf, sizeof(buf),
             "{\"seq\":%d,\"type\":\"event\",\"event\":\"output\",\"body\":{\"category\":\"%s\",\"output\":\"%s\"}}",
             dbg.dap_seq++, category ? category : "stdout", esc);
    dap_send_raw(buf);
}

static void dap_send_response(int request_seq, const char* command, bool success, const char* body_json) {
    char buf[8192];
    if (body_json && strlen(body_json) > 0) {
        snprintf(buf, sizeof(buf),
                 "{\"seq\":%d,\"type\":\"response\",\"request_seq\":%d,\"command\":\"%s\",\"success\":%s,\"body\":%s}",
                 dbg.dap_seq++, request_seq, command, success ? "true" : "false", body_json);
    } else {
        snprintf(buf, sizeof(buf),
                 "{\"seq\":%d,\"type\":\"response\",\"request_seq\":%d,\"command\":\"%s\",\"success\":%s}",
                 dbg.dap_seq++, request_seq, command, success ? "true" : "false");
    }
    dap_send_raw(buf);
}

static void dap_send_stopped_event(const char* reason, int line) {
    (void)line;
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"seq\":%d,\"type\":\"event\",\"event\":\"stopped\",\"body\":{\"reason\":\"%s\",\"threadId\":1,\"allThreadsStopped\":true}}",
             dbg.dap_seq++, reason);
    dap_send_raw(buf);
}

static char* dap_read_message(void) {
    char header[256];
    size_t content_length = 0;

    for (;;) {
        if (!fgets(header, sizeof(header), stdin)) return NULL;
        if (strncmp(header, "Content-Length:", 15) == 0) {
            content_length = (size_t)atoi(header + 15);
        } else if (strcmp(header, "\r\n") == 0 || strcmp(header, "\n") == 0) {
            break;
        }
    }

    if (content_length == 0) return NULL;

    char* content = (char*)malloc(content_length + 1);
    if (!content) return NULL;

    size_t total_read = 0;
    while (total_read < content_length) {
        size_t n = fread(content + total_read, 1, content_length - total_read, stdin);
        if (n == 0) break;
        total_read += n;
    }
    content[total_read] = '\0';
    return content;
}

static bool dap_handle_command(const char* json, Environment* env, int current_line, bool* out_resume) {
    *out_resume = false;

    // Quick command search in JSON
    const char* cmd_ptr = strstr(json, "\"command\":\"");
    if (!cmd_ptr) return true;
    cmd_ptr += 11;
    char cmd[64];
    size_t ci = 0;
    while (*cmd_ptr && *cmd_ptr != '"' && ci + 1 < sizeof(cmd)) {
        cmd[ci++] = *cmd_ptr++;
    }
    cmd[ci] = '\0';

    int seq = 1;
    const char* seq_ptr = strstr(json, "\"seq\":");
    if (seq_ptr) seq = atoi(seq_ptr + 6);

    if (strcmp(cmd, "threads") == 0) {
        dap_send_response(seq, cmd, true, "{\"threads\":[{\"id\":1,\"name\":\"Main Thread\"}]}");
    } else if (strcmp(cmd, "stackTrace") == 0) {
        char frames_json[4096];
        size_t fi = 0;
        fi += (size_t)snprintf(frames_json + fi, sizeof(frames_json) - fi, "{\"stackFrames\":[");

        int top_line = current_line;
        for (int i = dbg.stack_depth - 1; i >= 0; i--) {
            int l = (i == dbg.stack_depth - 1) ? top_line : dbg.stack[i].line;
            const char* fn = dbg.stack[i].function_name ? dbg.stack[i].function_name : "<script>";
            const char* path = dbg.stack[i].file ? dbg.stack[i].file : dbg.current_file;
            char safe_path[1024] = "";
            char safe_name[256] = "";
            dap_json_escape_path(path, safe_path, sizeof(safe_path));

            const char* b_base = strrchr(path, '/');
            const char* b_bs = strrchr(path, '\\');
            if (b_bs && (!b_base || b_bs > b_base)) b_base = b_bs;
            b_base = b_base ? b_base + 1 : path;
            dap_json_escape_path(b_base, safe_name, sizeof(safe_name));

            fi += (size_t)snprintf(frames_json + fi, sizeof(frames_json) - fi,
                                   "%s{\"id\":%d,\"name\":\"%s\",\"source\":{\"name\":\"%s\",\"path\":\"%s\"},\"line\":%d,\"column\":1}",
                                   (i == dbg.stack_depth - 1) ? "" : ",",
                                   i + 1, fn, safe_name, safe_path, l);
        }
        fi += (size_t)snprintf(frames_json + fi, sizeof(frames_json) - fi, "],\"totalFrames\":%d}", dbg.stack_depth);
        dap_send_response(seq, cmd, true, frames_json);
    } else if (strcmp(cmd, "scopes") == 0) {
        dap_send_response(seq, cmd, true,
                          "{\"scopes\":[{\"name\":\"Locals\",\"variablesReference\":1000,\"expensive\":false}]}");
    } else if (strcmp(cmd, "variables") == 0) {
        char vars_json[8192];
        size_t vi = 0;
        vi += (size_t)snprintf(vars_json + vi, sizeof(vars_json) - vi, "{\"variables\":[");
        size_t added = 0;

        Environment* curr_env = env;
        while (curr_env && vi + 256 < sizeof(vars_json)) {
            bool is_global = (curr_env->enclosing == NULL);
            for (size_t i = 0; i < curr_env->count && vi + 256 < sizeof(vars_json); i++) {
                const char* k = curr_env->entries[i].key;
                Value v = curr_env->entries[i].value;
                if (is_global && is_native(v)) continue;
                if (is_global && (strcmp(k, "cicekli") == 0 || strcmp(k, "mete") == 0 ||
                                  strcmp(k, "ccl") == 0 || strcmp(k, "cicekliscript") == 0 ||
                                  strcmp(k, "args") == 0 ||
                                  strcmp(k, "math") == 0 || strcmp(k, "file") == 0 ||
                                  strcmp(k, "doc") == 0 || strcmp(k, "cuda") == 0)) continue;
                char* v_str = value_to_string(v);
                char clean_val[256];
                size_t cvi = 0;
                for (size_t si = 0; v_str && v_str[si] && cvi + 2 < sizeof(clean_val); si++) {
                    if (v_str[si] == '"') { clean_val[cvi++] = '\\'; clean_val[cvi++] = '"'; }
                    else if (v_str[si] == '\n') { clean_val[cvi++] = ' '; }
                    else clean_val[cvi++] = v_str[si];
                }
                clean_val[cvi] = '\0';
                if (v_str) free(v_str);

                vi += (size_t)snprintf(vars_json + vi, sizeof(vars_json) - vi,
                                       "%s{\"name\":\"%s\",\"value\":\"%s\",\"type\":\"%s\",\"variablesReference\":0}",
                                       (added++ == 0) ? "" : ",", k, clean_val, value_type_name(v));
            }
            if (is_global) break;
            curr_env = curr_env->enclosing;
        }
        vi += (size_t)snprintf(vars_json + vi, sizeof(vars_json) - vi, "]}");
        dap_send_response(seq, cmd, true, vars_json);
    } else if (strcmp(cmd, "next") == 0) {
        dbg.step_mode = DEBUG_MODE_STEP_OVER;
        dbg.step_depth = dbg.stack_depth;
        dap_send_response(seq, cmd, true, NULL);
        *out_resume = true;
    } else if (strcmp(cmd, "stepIn") == 0) {
        dbg.step_mode = DEBUG_MODE_STEP_IN;
        dap_send_response(seq, cmd, true, NULL);
        *out_resume = true;
    } else if (strcmp(cmd, "stepOut") == 0) {
        dbg.step_mode = DEBUG_MODE_STEP_OUT;
        dbg.step_depth = dbg.stack_depth;
        dap_send_response(seq, cmd, true, NULL);
        *out_resume = true;
    } else if (strcmp(cmd, "continue") == 0) {
        dbg.step_mode = DEBUG_MODE_CONTINUE;
        dap_send_response(seq, cmd, true, "{\"allThreadsContinued\":true}");
        *out_resume = true;
    } else if (strcmp(cmd, "pause") == 0) {
        dbg.step_mode = DEBUG_MODE_STEP_IN;
        dap_send_response(seq, cmd, true, NULL);
    } else if (strcmp(cmd, "evaluate") == 0) {
        const char* expr_ptr = strstr(json, "\"expression\":\"");
        char expr_text[256] = "";
        if (expr_ptr) {
            expr_ptr += 14;
            size_t ei = 0;
            while (*expr_ptr && *expr_ptr != '"' && ei + 1 < sizeof(expr_text)) {
                expr_text[ei++] = *expr_ptr++;
            }
            expr_text[ei] = '\0';
        }

        Value out_val;
        if (env && env_get(env, expr_text, &out_val)) {
            char* val_str = value_to_string(out_val);
            char body[512];
            snprintf(body, sizeof(body),
                     "{\"result\":\"%s\",\"type\":\"%s\",\"variablesReference\":0}",
                     val_str ? val_str : "null", value_type_name(out_val));
            if (val_str) free(val_str);
            dap_send_response(seq, cmd, true, body);
        } else {
            dap_send_response(seq, cmd, true, "{\"result\":\"<undefined>\",\"variablesReference\":0}");
        }
    } else if (strcmp(cmd, "disconnect") == 0 || strcmp(cmd, "terminate") == 0) {
        dap_send_response(seq, cmd, true, NULL);
        exit(0);
    } else {
        dap_send_response(seq, cmd, true, NULL);
    }
    return true;
}

int debugger_run_dap(void) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    debugger_init(true);
    char program_path[1024] = "";
    bool stop_on_entry = true;

    // Phase 1: Wait for initialize, launch, setBreakpoints, configurationDone
    while (true) {
        char* msg = dap_read_message();
        if (!msg) return 1;

        const char* cmd_ptr = strstr(msg, "\"command\":\"");
        char cmd[64] = "";
        if (cmd_ptr) {
            cmd_ptr += 11;
            size_t ci = 0;
            while (*cmd_ptr && *cmd_ptr != '"' && ci + 1 < sizeof(cmd)) {
                cmd[ci++] = *cmd_ptr++;
            }
            cmd[ci] = '\0';
        }

        int seq = 1;
        const char* seq_ptr = strstr(msg, "\"seq\":");
        if (seq_ptr) seq = atoi(seq_ptr + 6);

        if (strcmp(cmd, "initialize") == 0) {
            dap_send_response(seq, cmd, true,
                              "{\"supportsConfigurationDoneRequest\":true,\"supportsEvaluateForHovers\":true}");
            char ev[256];
            snprintf(ev, sizeof(ev), "{\"seq\":%d,\"type\":\"event\",\"event\":\"initialized\"}", dbg.dap_seq++);
            dap_send_raw(ev);
        } else if (strcmp(cmd, "launch") == 0) {
            const char* prog_ptr = strstr(msg, "\"program\":\"");
            if (prog_ptr) {
                prog_ptr += 11;
                size_t pi = 0;
                while (*prog_ptr && *prog_ptr != '"' && pi + 1 < sizeof(program_path)) {
                    if (*prog_ptr == '\\' && *(prog_ptr + 1) == '\\') prog_ptr++;
                    program_path[pi++] = *prog_ptr++;
                }
                program_path[pi] = '\0';
            }
            if (strstr(msg, "\"stopOnEntry\":false")) {
                stop_on_entry = false;
            }
            dbg.stop_on_entry = stop_on_entry;
            dap_send_response(seq, cmd, true, NULL);
        } else if (strcmp(cmd, "setBreakpoints") == 0) {
            char bp_file[512] = "";
            const char* path_ptr = strstr(msg, "\"path\":\"");
            if (path_ptr) {
                path_ptr += 8;
                size_t pi = 0;
                while (*path_ptr && *path_ptr != '"' && pi + 1 < sizeof(bp_file)) {
                    if (*path_ptr == '\\' && *(path_ptr + 1) == '\\') path_ptr++;
                    bp_file[pi++] = *path_ptr++;
                }
                bp_file[pi] = '\0';
            }

            char res_body[4096] = "{\"breakpoints\":[";
            size_t rbi = strlen(res_body);
            size_t bp_added = 0;

            const char* lines_ptr = strstr(msg, "\"lines\":[");
            if (lines_ptr) {
                lines_ptr += 9;
                while (*lines_ptr && *lines_ptr != ']') {
                    while (*lines_ptr && !isdigit((unsigned char)*lines_ptr) && *lines_ptr != ']') lines_ptr++;
                    if (isdigit((unsigned char)*lines_ptr)) {
                        int line_num = atoi(lines_ptr);
                        debugger_add_breakpoint(bp_file, line_num);
                        rbi += (size_t)snprintf(res_body + rbi, sizeof(res_body) - rbi,
                                                "%s{\"verified\":true,\"line\":%d}",
                                                (bp_added++ == 0) ? "" : ",", line_num);
                        while (*lines_ptr && isdigit((unsigned char)*lines_ptr)) lines_ptr++;
                    }
                }
            }
            strcat(res_body, "]}");
            dap_send_response(seq, cmd, true, res_body);
        } else if (strcmp(cmd, "setExceptionBreakpoints") == 0) {
            dap_send_response(seq, cmd, true, NULL);
        } else if (strcmp(cmd, "configurationDone") == 0) {
            dap_send_response(seq, cmd, true, NULL);
            free(msg);
            break;
        } else if (strcmp(cmd, "disconnect") == 0 || strcmp(cmd, "terminate") == 0) {
            dap_send_response(seq, cmd, true, NULL);
            free(msg);
            return 0;
        } else {
            dap_send_response(seq, cmd, true, NULL);
        }

        free(msg);
    }

    if (program_path[0] == '\0') {
        return 1;
    }

    FILE* f = fopen(program_path, "rb");
    if (!f) return 1;
    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* src = (char*)malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    debugger_set_source(program_path, src);

    Parser parser;
    parser_init(&parser, src);
    size_t count = 0;
    Stmt** stmts = parse_program(&parser, &count);

    if (parser.had_error) {
        free(stmts);
        free(src);
        return 1;
    }

    Environment* global_env = init_global_environment();
    debugger_push_frame("<script>", program_path, 1, global_env);

    EvalResult res = eval_statements(stmts, count, global_env);

    char term_ev[256];
    snprintf(term_ev, sizeof(term_ev), "{\"seq\":%d,\"type\":\"event\",\"event\":\"terminated\"}", dbg.dap_seq++);
    dap_send_raw(term_ev);

    char exit_ev[256];
    snprintf(exit_ev, sizeof(exit_ev),
             "{\"seq\":%d,\"type\":\"event\",\"event\":\"exited\",\"body\":{\"exitCode\":%d}}",
             dbg.dap_seq++, res.type == RES_ERROR ? 1 : 0);
    dap_send_raw(exit_ev);

    free(stmts);
    free(src);
    return res.type == RES_ERROR ? 1 : 0;
}

/* ========================================================================= */
/* Core Statement Interceptor                                                */
/* ========================================================================= */

void debugger_on_statement(Stmt* stmt, Environment* env) {
    if (!dbg.active || !stmt || stmt->line <= 0) return;

    // Filter sub-statements or block wrapper repeats on the exact same line
    if (stmt->type == STMT_BLOCK) return;
    if (stmt->line == dbg.last_stop_line) return;

    bool should_stop = false;
    const char* stop_reason = "step";

    // 1. Check entry stop
    if (dbg.stop_on_entry && !dbg.has_stopped_entry) {
        dbg.has_stopped_entry = true;
        should_stop = true;
        stop_reason = "entry";
    }

    // 2. Check line breakpoint
    Breakpoint* bp = find_breakpoint(dbg.current_file, stmt->line);
    if (bp) {
        bp->hit_count++;
        should_stop = true;
        stop_reason = "breakpoint";
    }

    // 3. Check stepping mode
    if (!should_stop) {
        if (dbg.step_mode == DEBUG_MODE_STEP_IN) {
            should_stop = true;
            stop_reason = "step";
        } else if (dbg.step_mode == DEBUG_MODE_STEP_OVER && dbg.stack_depth <= dbg.step_depth) {
            should_stop = true;
            stop_reason = "step";
        } else if (dbg.step_mode == DEBUG_MODE_STEP_OUT && dbg.stack_depth < dbg.step_depth) {
            should_stop = true;
            stop_reason = "step";
        }
    }

    if (!should_stop) return;

    dbg.last_stop_line = stmt->line;
    debugger_update_frame_line(stmt->line);

    /* --- Mode A: VS Code DAP Mode --- */
    if (dbg.is_dap) {
        dap_send_stopped_event(stop_reason, stmt->line);
        for (;;) {
            char* msg = dap_read_message();
            if (!msg) break;
            bool resume = false;
            dap_handle_command(msg, env, stmt->line, &resume);
            free(msg);
            if (resume) break;
        }
        return;
    }

    /* --- Mode B: Interactive English CLI Debugger --- */
    const char* fn_name = (dbg.stack_depth > 0 && dbg.stack[dbg.stack_depth - 1].function_name)
                              ? dbg.stack[dbg.stack_depth - 1].function_name
                              : "main";

    if (strcmp(stop_reason, "breakpoint") == 0 && bp) {
        printf("\n\033[1;33m[Breakpoint %d Hit]\033[0m %s:%d in function '%s'\n",
               bp->id, dbg.current_file ? dbg.current_file : "<file>", stmt->line, fn_name);
    } else {
        printf("\n\033[1;36m[Stopped (%s)]\033[0m %s:%d in function '%s'\n",
               stop_reason, dbg.current_file ? dbg.current_file : "<file>", stmt->line, fn_name);
    }

    print_source_snippet(stmt->line, 2);

    char input[512];
    static char last_cmd[64] = "n";

    for (;;) {
        printf("\033[1;32m(ccl-dbg)\033[0m ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            exit(0);
        }

        // Strip trailing whitespace
        size_t len = strlen(input);
        while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r' || input[len - 1] == ' ')) {
            input[--len] = '\0';
        }

        // Repeat last command on empty enter (for s, n, c)
        if (len == 0) {
            strncpy(input, last_cmd, sizeof(input) - 1);
        }

        // Skip leading spaces
        char* cmd = input;
        while (*cmd && isspace((unsigned char)*cmd)) cmd++;
        if (*cmd == '\0') continue;

        if (strcmp(cmd, "s") == 0 || strcmp(cmd, "step") == 0 || strcmp(cmd, "stepin") == 0) {
            strcpy(last_cmd, "s");
            dbg.step_mode = DEBUG_MODE_STEP_IN;
            break;
        } else if (strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0 || strcmp(cmd, "stepover") == 0) {
            strcpy(last_cmd, "n");
            dbg.step_mode = DEBUG_MODE_STEP_OVER;
            dbg.step_depth = dbg.stack_depth;
            break;
        } else if (strcmp(cmd, "out") == 0 || strcmp(cmd, "finish") == 0 || strcmp(cmd, "stepout") == 0) {
            strcpy(last_cmd, "out");
            dbg.step_mode = DEBUG_MODE_STEP_OUT;
            dbg.step_depth = dbg.stack_depth;
            break;
        } else if (strcmp(cmd, "c") == 0 || strcmp(cmd, "continue") == 0) {
            strcpy(last_cmd, "c");
            dbg.step_mode = DEBUG_MODE_CONTINUE;
            break;
        } else if (strncmp(cmd, "b ", 2) == 0 || strncmp(cmd, "break ", 6) == 0) {
            char* arg = cmd + (cmd[1] == ' ' ? 2 : 6);
            while (*arg && isspace((unsigned char)*arg)) arg++;
            char* colon = strchr(arg, ':');
            if (colon) {
                *colon = '\0';
                int line_num = atoi(colon + 1);
                int id = debugger_add_breakpoint(arg, line_num);
                printf("Breakpoint %d set at %s:%d\n", id, arg, line_num);
            } else {
                int line_num = atoi(arg);
                if (line_num > 0) {
                    int id = debugger_add_breakpoint(dbg.current_file, line_num);
                    printf("Breakpoint %d set at %s:%d\n", id, dbg.current_file ? dbg.current_file : "<file>", line_num);
                } else {
                    printf("Invalid line number: %s\n", arg);
                }
            }
        } else if (strncmp(cmd, "del", 3) == 0) {
            char* arg = cmd + 3;
            while (*arg && isspace((unsigned char)*arg)) arg++;
            if (*arg == '\0') {
                debugger_clear_breakpoints();
                printf("All breakpoints deleted.\n");
            } else {
                int id = atoi(arg);
                if (debugger_remove_breakpoint(id)) {
                    printf("Breakpoint %d deleted.\n", id);
                } else {
                    printf("No breakpoint found with ID %d.\n", id);
                }
            }
        } else if (strcmp(cmd, "info b") == 0 || strcmp(cmd, "breakpoints") == 0) {
            debugger_list_breakpoints();
        } else if (strcmp(cmd, "locals") == 0 || strcmp(cmd, "info locals") == 0) {
            printf("Local variables in '%s':\n", fn_name);
            print_locals(env);
        } else if (strncmp(cmd, "p ", 2) == 0 || strncmp(cmd, "print ", 6) == 0) {
            char* expr_str = cmd + (cmd[1] == ' ' ? 2 : 6);
            while (*expr_str && isspace((unsigned char)*expr_str)) expr_str++;
            Value val;
            if (env && env_get(env, expr_str, &val)) {
                char* s = value_to_string(val);
                printf("$ = %s  [%s]\n", s ? s : "null", value_type_name(val));
                if (s) free(s);
            } else {
                char eval_code[512];
                snprintf(eval_code, sizeof(eval_code), "%s;", expr_str);
                Parser parser;
                parser_init(&parser, eval_code);
                Stmt* parsed_stmt = parse_statement(&parser);
                if (parsed_stmt && !parser.had_error) {
                    if (parsed_stmt->type == STMT_EXPR) {
                        bool err = false;
                        Value res = eval_expression(parsed_stmt->as.expr, env, &err);
                        if (!err) {
                            char* s = value_to_string(res);
                            printf("$ = %s  [%s]\n", s ? s : "null", value_type_name(res));
                            if (s) free(s);
                        } else {
                            printf("Evaluation error for: %s\n", expr_str);
                        }
                    } else {
                        EvalResult res = eval_statement(parsed_stmt, env);
                        if (res.type != RES_ERROR) {
                            printf("Statement executed successfully.\n");
                        }
                    }
                } else {
                    printf("Unknown variable or syntax error in expression: '%s'\n", expr_str);
                }
            }
        } else if (strcmp(cmd, "bt") == 0 || strcmp(cmd, "backtrace") == 0 || strcmp(cmd, "stack") == 0) {
            print_backtrace(stmt->line);
        } else if (strncmp(cmd, "l", 1) == 0) {
            int span = 5;
            char* arg = cmd + 1;
            while (*arg && isspace((unsigned char)*arg)) arg++;
            if (*arg) span = atoi(arg);
            if (span < 1) span = 5;
            print_source_snippet(stmt->line, span);
        } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            printf("Debugger session terminated.\n");
            exit(0);
        } else {
            printf("Unknown command: '%s'. Type 'help' for available commands.\n", cmd);
        }
    }
}
