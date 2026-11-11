#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gc.h"
#include "gui.h"
#include "parser.h"
#include "eval.h"
#include "debug.h"

#define CICEKLISCRIPT_VERSION "1.2.0"

static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "CicekliScript Hatası: '%s' dosyası açılamadı!\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "CicekliScript Hatası: Bellek ayrılamadı!\n");
        return NULL;
    }

    size_t read_bytes = fread(buffer, sizeof(char), size, file);
    buffer[read_bytes] = '\0';
    fclose(file);
    return buffer;
}

static int run_source(const char* source, Environment* env) {
    Parser parser;
    parser_init(&parser, source);

    size_t count = 0;
    Stmt** stmts = parse_program(&parser, &count);

    if (parser.had_error) {
        free(stmts);
        return 1;
    }

    EvalResult res = eval_statements(stmts, count, env);
    free(stmts);

    return res.type == RES_ERROR ? 1 : 0;
}

static void run_repl(Environment* env) {
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║          CicekliScript v%s REPL                 ║\n", CICEKLISCRIPT_VERSION);
    printf("║  Python kolaylığı + C gücü + Zorunlu ';'         ║\n");
    printf("║  Çıkmak için: exit() veya cicekli.exit();        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");

    char buffer[4096];
    char total_input[16384];
    total_input[0] = '\0';

    for (;;) {
        if (total_input[0] == '\0') {
            printf("cicekli>>> ");
        } else {
            printf("cicekli... ");
        }
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("\n");
            break;
        }

        // Check for quit
        if (strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "quit\n") == 0 ||
            strcmp(buffer, "exit()\n") == 0 || strcmp(buffer, "exit();\n") == 0) {
            break;
        }

        strcat(total_input, buffer);

        // Check if brackets/braces are balanced
        int braces = 0;
        int brackets = 0;
        int parens = 0;
        bool in_str = false;
        char quote = '\0';

        for (size_t i = 0; total_input[i]; i++) {
            char c = total_input[i];
            if ((c == '"' || c == '\'') && (i == 0 || total_input[i - 1] != '\\')) {
                if (!in_str) {
                    in_str = true;
                    quote = c;
                } else if (c == quote) {
                    in_str = false;
                }
            } else if (!in_str) {
                if (c == '{') braces++;
                else if (c == '}') braces--;
                else if (c == '[') brackets++;
                else if (c == ']') brackets--;
                else if (c == '(') parens++;
                else if (c == ')') parens--;
            }
        }

        // If open blocks remain, continue reading lines
        if (braces > 0 || brackets > 0 || parens > 0) {
            continue;
        }

        // Try evaluating as an expression first for convenient REPL output
        // e.g. typing "2 + 2;" or "mete.versiyon;" will print the value
        size_t len = strlen(total_input);
        while (len > 0 && (total_input[len - 1] == '\n' || total_input[len - 1] == ' ' || total_input[len - 1] == '\r')) {
            total_input[--len] = '\0';
        }
        if (len == 0) {
            total_input[0] = '\0';
            continue;
        }

        // If line didn't end with ';', add it automatically in REPL for convenience if it's an expression!
        if (total_input[len - 1] != ';' && total_input[len - 1] != '}') {
            strcat(total_input, ";\n");
        } else {
            strcat(total_input, "\n");
        }

        run_source(total_input, env);
        total_input[0] = '\0';
    }
}

static void print_help(const char* prog) {
    printf("Usage:\n");
    printf("  %s                         Start interactive REPL\n", prog);
    printf("  %s <file.ccl>             Run CicekliScript file\n", prog);
    printf("  %s -c \"<code>\"            Execute inline CicekliScript code\n", prog);
    printf("  %s --debug, -d <file.ccl> Start interactive CLI debugger\n", prog);
    printf("  %s --dap                   Start Debug Adapter Protocol (for VS Code)\n", prog);
    printf("  %s --version, -v           Show CicekliScript version\n", prog);
    printf("  %s --help, -h              Show this help menu\n", prog);
}

int main(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "--dap") == 0) {
        gc_init();
        ast_init();
        int ret = debugger_run_dap();
        ast_free_all();
        gc_free_all();
        gui_cleanup();
        return ret;
    }

    gc_init();
    ast_init();
    Environment* global_env = init_global_environment();

    if (argc == 1) {
        run_repl(global_env);
    } else if (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("CicekliScript v%s\n", CICEKLISCRIPT_VERSION);
            return 0;
        } else if (strcmp(argv[1], "--debug") == 0 || strcmp(argv[1], "-d") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Error: Missing source file to debug. Usage: %s --debug <file.ccl>\n", argv[0]);
                return 1;
            }
            const char* filename = argv[2];
            eval_set_args(global_env, argc, argv, 2);
            char* source = read_file(filename);
            if (!source) return 1;

            debugger_init(false);
            debugger_set_source(filename, source);
            debugger_push_frame("<script>", filename, 1, global_env);

            printf("\n\033[1;35m=== CicekliScript Debugger v%s ===\033[0m\n", CICEKLISCRIPT_VERSION);
            printf("Type 'help' or 'h' for commands, 's'/'n' to step, 'c' to run.\n");

            int ret = run_source(source, global_env);
            debugger_cleanup();
            free(source);
            ast_free_all();
            gc_free_all();
            gui_cleanup();
            return ret;
        } else if (strcmp(argv[1], "-c") == 0) {
            if (argc < 3) {
                print_help(argv[0]);
                return 1;
            }
            eval_set_args(global_env, argc, argv, 2);
            int ret = run_source(argv[2], global_env);
            ast_free_all();
            gc_free_all();
            gui_cleanup();
            return ret;
        } else {
            eval_set_args(global_env, argc, argv, 1);
            char* source = read_file(argv[1]);
            if (!source) return 1;
            int ret = run_source(source, global_env);
            free(source);
            ast_free_all();
            gc_free_all();
            gui_cleanup();
            return ret;
        }
    }

    ast_free_all();
    gc_free_all();
    gui_cleanup();
    return 0;
}
