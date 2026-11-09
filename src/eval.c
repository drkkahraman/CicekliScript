#include "eval.h"
#include "gc.h"
#include "gui.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

static void runtime_error(int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "CicekliScript Çalışma Zamanı Hatası (Satır %d): ", line);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

// Built-in functions
static Value native_println(Value self, size_t arg_count, Value* args, int line) {
    (void)self; (void)line;
    for (size_t i = 0; i < arg_count; i++) {
        print_value(args[i]);
        if (i + 1 < arg_count) {
            putchar(' ');
        }
    }
    putchar('\n');
    fflush(stdout);
    return make_null();
}

static Value native_print(Value self, size_t arg_count, Value* args, int line) {
    (void)self; (void)line;
    for (size_t i = 0; i < arg_count; i++) {
        print_value(args[i]);
        if (i + 1 < arg_count) {
            putchar(' ');
        }
    }
    fflush(stdout);
    return make_null();
}

static Value native_input(Value self, size_t arg_count, Value* args, int line) {
    (void)self; (void)line;
    if (arg_count > 0) {
        print_value(args[0]);
        fflush(stdout);
    }
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len - 1] == '\r') {
            buffer[len - 1] = '\0';
            len--;
        }
        return make_obj((Obj*)copy_string(buffer));
    }
    return make_obj((Obj*)copy_string(""));
}

static Value native_len(Value self, size_t arg_count, Value* args, int line) {
    Value v = self;
    if (is_null(v) || (is_map(v) && arg_count == 1)) {
        if (arg_count != 1) {
            runtime_error(line, "len() tam olarak 1 argüman bekler!");
            return make_null();
        }
        v = args[0];
    }
    if (is_string(v)) {
        return make_number((double)as_string(v)->length);
    }
    if (is_list(v)) {
        return make_number((double)as_list(v)->count);
    }
    if (is_map(v)) {
        return make_number((double)as_map(v)->count);
    }
    runtime_error(line, "'%s' türünün uzunluğu (len) alınamaz!", value_type_name(v));
    return make_null();
}

static Value native_range(Value self, size_t arg_count, Value* args, int line) {
    (void)self;
    double start = 0;
    double end = 0;
    double step = 1;

    if (arg_count == 1) {
        if (!is_number(args[0])) {
            runtime_error(line, "range() sayı parametresi bekler!");
            return make_null();
        }
        end = as_number(args[0]);
    } else if (arg_count == 2) {
        if (!is_number(args[0]) || !is_number(args[1])) {
            runtime_error(line, "range() sayı parametreleri bekler!");
            return make_null();
        }
        start = as_number(args[0]);
        end = as_number(args[1]);
    } else if (arg_count == 3) {
        if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
            runtime_error(line, "range() sayı parametreleri bekler!");
            return make_null();
        }
        start = as_number(args[0]);
        end = as_number(args[1]);
        step = as_number(args[2]);
        if (step == 0) {
            runtime_error(line, "range() adımı 0 olamaz!");
            return make_null();
        }
    } else {
        runtime_error(line, "range() 1, 2 veya 3 argüman bekler!");
        return make_null();
    }

    ObjList* list = new_list();
    if (step > 0) {
        for (double i = start; i < end; i += step) {
            list_append(list, make_number(i));
        }
    } else {
        for (double i = start; i > end; i += step) {
            list_append(list, make_number(i));
        }
    }
    return make_obj((Obj*)list);
}

static Value native_append(Value self, size_t arg_count, Value* args, int line) {
    Value list_val = self;
    Value item = make_null();
    if (!is_list(list_val)) {
        if (arg_count != 2) {
            runtime_error(line, "append() 2 argüman (liste, eleman) bekler!");
            return make_null();
        }
        list_val = args[0];
        item = args[1];
    } else {
        if (arg_count != 1) {
            runtime_error(line, "list.append() 1 argüman bekler!");
            return make_null();
        }
        item = args[0];
    }
    if (!is_list(list_val)) {
        runtime_error(line, "append() liste bekler!");
        return make_null();
    }
    list_append(as_list(list_val), item);
    return list_val;
}

static Value native_pop(Value self, size_t arg_count, Value* args, int line) {
    Value list_val = self;
    int64_t idx = -1;
    if (!is_list(list_val)) {
        if (arg_count < 1 || arg_count > 2) {
            runtime_error(line, "pop() 1 veya 2 argüman bekler!");
            return make_null();
        }
        list_val = args[0];
        if (arg_count == 2) {
            if (!is_number(args[1])) {
                runtime_error(line, "pop() indeks olarak sayı bekler!");
                return make_null();
            }
            idx = (int64_t)as_number(args[1]);
        }
    } else {
        if (arg_count > 1) {
            runtime_error(line, "list.pop() en fazla 1 argüman bekler!");
            return make_null();
        }
        if (arg_count == 1) {
            if (!is_number(args[0])) {
                runtime_error(line, "pop() indeks olarak sayı bekler!");
                return make_null();
            }
            idx = (int64_t)as_number(args[0]);
        }
    }
    if (!is_list(list_val)) {
        runtime_error(line, "pop() liste bekler!");
        return make_null();
    }
    return list_pop(as_list(list_val), idx);
}

static Value native_type(Value self, size_t arg_count, Value* args, int line) {
    (void)self;
    if (arg_count != 1) {
        runtime_error(line, "type() tam olarak 1 argüman bekler!");
        return make_null();
    }
    const char* tname = value_type_name(args[0]);
    return make_obj((Obj*)copy_string(tname));
}

static Value native_str(Value self, size_t arg_count, Value* args, int line) {
    (void)self;
    if (arg_count != 1) {
        runtime_error(line, "str() tam olarak 1 argüman bekler!");
        return make_null();
    }
    char* s = value_to_string(args[0]);
    ObjString* str_obj = copy_string(s);
    free(s);
    return make_obj((Obj*)str_obj);
}

static Value native_int(Value self, size_t arg_count, Value* args, int line) {
    (void)self;
    if (arg_count != 1) {
        runtime_error(line, "int() tam olarak 1 argüman bekler!");
        return make_null();
    }
    Value v = args[0];
    if (is_number(v)) {
        return make_number(floor(as_number(v)));
    }
    if (is_string(v)) {
        return make_number(floor(strtod(as_string(v)->chars, NULL)));
    }
    if (is_bool(v)) {
        return make_number(as_bool(v) ? 1.0 : 0.0);
    }
    runtime_error(line, "'%s' türü tam sayıya dönüştürülemez!", value_type_name(v));
    return make_null();
}

static Value native_float(Value self, size_t arg_count, Value* args, int line) {
    (void)self;
    if (arg_count != 1) {
        runtime_error(line, "float() tam olarak 1 argüman bekler!");
        return make_null();
    }
    Value v = args[0];
    if (is_number(v)) {
        return v;
    }
    if (is_string(v)) {
        return make_number(strtod(as_string(v)->chars, NULL));
    }
    if (is_bool(v)) {
        return make_number(as_bool(v) ? 1.0 : 0.0);
    }
    runtime_error(line, "'%s' türü kesirli sayıya dönüştürülemez!", value_type_name(v));
    return make_null();
}

static Value native_time(Value self, size_t arg_count, Value* args, int line) {
    (void)self; (void)arg_count; (void)args; (void)line;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    double sec = (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
    return make_number(sec);
}

static Value native_sleep(Value self, size_t arg_count, Value* args, int line) {
    (void)self;
    if (arg_count < 1 || !is_number(args[0])) {
        runtime_error(line, "sleep() sayısal bir süre parametresi bekler (saniye cinsinden, örn: sleep(1) veya sleep(0.5))!");
        return make_null();
    }
    double seconds = as_number(args[0]);
    if (seconds > 0) {
        struct timespec req;
        req.tv_sec = (time_t)seconds;
        req.tv_nsec = (long)((seconds - (double)req.tv_sec) * 1e9);
        nanosleep(&req, NULL);
    }
    return make_null();
}

static Value native_exit(Value self, size_t arg_count, Value* args, int line) {
    (void)self; (void)line;
    int code = 0;
    if (arg_count > 0 && is_number(args[0])) {
        code = (int)as_number(args[0]);
    }
    exit(code);
    return make_null();
}

static Environment* g_active_env = NULL;

// --- Math Functions ---
static Value native_sqrt(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "sqrt() expects a number!");
        return make_null();
    }
    double n = as_number(args[0]);
    if (n < 0) {
        runtime_error(line, "sqrt() domain error (negative number)!");
        return make_null();
    }
    return make_number(sqrt(n));
}

static Value native_pow(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 2 || !is_number(args[0]) || !is_number(args[1])) {
        runtime_error(line, "pow() expects two numbers (base, exp)!");
        return make_null();
    }
    return make_number(pow(as_number(args[0]), as_number(args[1])));
}

static Value native_abs(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "abs() expects a number!");
        return make_null();
    }
    return make_number(fabs(as_number(args[0])));
}

static Value native_round(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "round() expects a number!");
        return make_null();
    }
    return make_number(round(as_number(args[0])));
}

static Value native_floor(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "floor() expects a number!");
        return make_null();
    }
    return make_number(floor(as_number(args[0])));
}

static Value native_ceil(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "ceil() expects a number!");
        return make_null();
    }
    return make_number(ceil(as_number(args[0])));
}

static Value native_sin(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "sin() expects a number in radians!");
        return make_null();
    }
    return make_number(sin(as_number(args[0])));
}

static Value native_cos(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "cos() expects a number in radians!");
        return make_null();
    }
    return make_number(cos(as_number(args[0])));
}

static Value native_tan(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "tan() expects a number in radians!");
        return make_null();
    }
    return make_number(tan(as_number(args[0])));
}

static Value native_log(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_number(args[0])) {
        runtime_error(line, "log() expects a number!");
        return make_null();
    }
    double n = as_number(args[0]);
    if (n <= 0) {
        runtime_error(line, "log() domain error (non-positive number)!");
        return make_null();
    }
    return make_number(log(n));
}

static Value native_min(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc == 0) {
        runtime_error(line, "min() expects at least 1 argument!");
        return make_null();
    }
    if (argc == 1 && is_list(args[0])) {
        ObjList* l = as_list(args[0]);
        if (l->count == 0) return make_null();
        double m = is_number(l->items[0]) ? as_number(l->items[0]) : 0.0;
        for (size_t i = 1; i < l->count; i++) {
            if (is_number(l->items[i]) && as_number(l->items[i]) < m) {
                m = as_number(l->items[i]);
            }
        }
        return make_number(m);
    }
    if (!is_number(args[0])) return make_null();
    double m = as_number(args[0]);
    for (size_t i = 1; i < argc; i++) {
        if (is_number(args[i]) && as_number(args[i]) < m) {
            m = as_number(args[i]);
        }
    }
    return make_number(m);
}

static Value native_max(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc == 0) {
        runtime_error(line, "max() expects at least 1 argument!");
        return make_null();
    }
    if (argc == 1 && is_list(args[0])) {
        ObjList* l = as_list(args[0]);
        if (l->count == 0) return make_null();
        double m = is_number(l->items[0]) ? as_number(l->items[0]) : 0.0;
        for (size_t i = 1; i < l->count; i++) {
            if (is_number(l->items[i]) && as_number(l->items[i]) > m) {
                m = as_number(l->items[i]);
            }
        }
        return make_number(m);
    }
    if (!is_number(args[0])) return make_null();
    double m = as_number(args[0]);
    for (size_t i = 1; i < argc; i++) {
        if (is_number(args[i]) && as_number(args[i]) > m) {
            m = as_number(args[i]);
        }
    }
    return make_number(m);
}

static Value native_random(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)line;
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    if (argc == 0) {
        return make_number((double)rand() / ((double)RAND_MAX + 1.0));
    }
    if (argc == 1 && is_number(args[0])) {
        double max_val = as_number(args[0]);
        return make_number((double)rand() / (double)RAND_MAX * max_val);
    }
    if (argc >= 2 && is_number(args[0]) && is_number(args[1])) {
        double min_val = as_number(args[0]);
        double max_val = as_number(args[1]);
        return make_number(min_val + ((double)rand() / (double)RAND_MAX) * (max_val - min_val));
    }
    return make_number((double)rand() / ((double)RAND_MAX + 1.0));
}

// --- File I/O Functions ---
static Value native_read_file(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_string(args[0])) {
        runtime_error(line, "read_file() expects a string file path!");
        return make_null();
    }
    const char* path = as_string(args[0])->chars;
    FILE* f = fopen(path, "rb");
    if (!f) return make_null();

    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return make_null();
    }
    size_t read_bytes = fread(buffer, sizeof(char), size, f);
    buffer[read_bytes] = '\0';
    fclose(f);

    ObjString* str = take_string(buffer, read_bytes);
    return make_obj((Obj*)str);
}

static Value native_write_file(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 2 || !is_string(args[0])) {
        runtime_error(line, "write_file(path, content) expects path and content!");
        return make_bool(false);
    }
    const char* path = as_string(args[0])->chars;
    char* content = value_to_string(args[1]);
    FILE* f = fopen(path, "wb");
    if (!f) {
        free(content);
        return make_bool(false);
    }
    fputs(content, f);
    fclose(f);
    free(content);
    return make_bool(true);
}

static Value native_append_file(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 2 || !is_string(args[0])) {
        runtime_error(line, "append_file(path, content) expects path and content!");
        return make_bool(false);
    }
    const char* path = as_string(args[0])->chars;
    char* content = value_to_string(args[1]);
    FILE* f = fopen(path, "ab");
    if (!f) {
        free(content);
        return make_bool(false);
    }
    fputs(content, f);
    fclose(f);
    free(content);
    return make_bool(true);
}

static Value native_file_exists(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_string(args[0])) {
        runtime_error(line, "file_exists() expects a string file path!");
        return make_bool(false);
    }
    const char* path = as_string(args[0])->chars;
    return make_bool(access(path, F_OK) == 0);
}

static Value native_delete_file(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_string(args[0])) {
        runtime_error(line, "delete_file() expects a string file path!");
        return make_bool(false);
    }
    const char* path = as_string(args[0])->chars;
    return make_bool(remove(path) == 0);
}

// --- String & List Helpers ---
static Value native_split(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    const char* delim = " ";
    if (is_null(target)) {
        if (argc < 1 || !is_string(args[0])) {
            runtime_error(line, "split() expects a string to split!");
            return make_null();
        }
        target = args[0];
        if (argc >= 2 && is_string(args[1])) {
            delim = as_string(args[1])->chars;
        }
    } else {
        if (!is_string(target)) return make_null();
        if (argc >= 1 && is_string(args[0])) {
            delim = as_string(args[0])->chars;
        }
    }

    ObjString* str = as_string(target);
    ObjList* result = new_list();
    gc_push_root(make_obj((Obj*)result));

    size_t dlen = strlen(delim);
    if (dlen == 0) {
        for (size_t i = 0; i < str->length; i++) {
            char ch[2] = { str->chars[i], '\0' };
            list_append(result, make_obj((Obj*)copy_string(ch)));
        }
    } else {
        const char* p = str->chars;
        const char* end = str->chars + str->length;
        while (p <= end) {
            const char* next = strstr(p, delim);
            if (!next) {
                list_append(result, make_obj((Obj*)copy_string(p)));
                break;
            } else {
                size_t part_len = next - p;
                char* buf = (char*)malloc(part_len + 1);
                memcpy(buf, p, part_len);
                buf[part_len] = '\0';
                list_append(result, make_obj((Obj*)take_string(buf, part_len)));
                p = next + dlen;
            }
        }
    }

    gc_pop_roots(1);
    return make_obj((Obj*)result);
}

static Value native_join(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    const char* delim = "";
    if (is_null(target)) {
        if (argc < 1 || !is_list(args[0])) {
            runtime_error(line, "join() expects a list!");
            return make_null();
        }
        target = args[0];
        if (argc >= 2 && is_string(args[1])) {
            delim = as_string(args[1])->chars;
        }
    } else {
        if (!is_list(target)) return make_null();
        if (argc >= 1 && is_string(args[0])) {
            delim = as_string(args[0])->chars;
        }
    }

    ObjList* list = as_list(target);
    if (list->count == 0) return make_obj((Obj*)copy_string(""));

    size_t dlen = strlen(delim);
    size_t total_len = 0;
    char** parts = (char**)malloc(sizeof(char*) * list->count);
    size_t* lens = (size_t*)malloc(sizeof(size_t) * list->count);

    for (size_t i = 0; i < list->count; i++) {
        parts[i] = value_to_string(list->items[i]);
        lens[i] = strlen(parts[i]);
        total_len += lens[i];
        if (i + 1 < list->count) total_len += dlen;
    }

    char* res = (char*)malloc(total_len + 1);
    char* cur = res;
    for (size_t i = 0; i < list->count; i++) {
        memcpy(cur, parts[i], lens[i]);
        cur += lens[i];
        if (i + 1 < list->count && dlen > 0) {
            memcpy(cur, delim, dlen);
            cur += dlen;
        }
        free(parts[i]);
    }
    *cur = '\0';
    free(parts);
    free(lens);

    return make_obj((Obj*)take_string(res, total_len));
}

static Value native_trim(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_string(args[0])) {
            runtime_error(line, "trim() expects a string!");
            return make_null();
        }
        target = args[0];
    } else if (!is_string(target)) {
        return make_null();
    }

    ObjString* str = as_string(target);
    const char* s = str->chars;
    size_t len = str->length;
    while (len > 0 && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) {
        s++;
        len--;
    }
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\n' || s[len - 1] == '\r')) {
        len--;
    }
    char* buf = (char*)malloc(len + 1);
    memcpy(buf, s, len);
    buf[len] = '\0';
    return make_obj((Obj*)take_string(buf, len));
}

static Value native_replace(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    const char *old_sub = NULL, *new_sub = NULL;
    if (is_null(target)) {
        if (argc < 3 || !is_string(args[0]) || !is_string(args[1]) || !is_string(args[2])) {
            runtime_error(line, "replace(str, old, new) expects 3 strings!");
            return make_null();
        }
        target = args[0];
        old_sub = as_string(args[1])->chars;
        new_sub = as_string(args[2])->chars;
    } else {
        if (!is_string(target) || argc < 2 || !is_string(args[0]) || !is_string(args[1])) {
            runtime_error(line, "replace(old, new) expects 2 strings!");
            return make_null();
        }
        old_sub = as_string(args[0])->chars;
        new_sub = as_string(args[1])->chars;
    }

    ObjString* src = as_string(target);
    size_t old_len = strlen(old_sub);
    size_t new_len = strlen(new_sub);
    if (old_len == 0) return make_obj((Obj*)copy_string(src->chars));

    size_t count = 0;
    const char* pos = src->chars;
    while ((pos = strstr(pos, old_sub)) != NULL) {
        count++;
        pos += old_len;
    }

    size_t new_total_len = src->length + count * (new_len - old_len);
    char* result = (char*)malloc(new_total_len + 1);

    const char* read_ptr = src->chars;
    char* write_ptr = result;
    while ((pos = strstr(read_ptr, old_sub)) != NULL) {
        size_t bytes = pos - read_ptr;
        memcpy(write_ptr, read_ptr, bytes);
        write_ptr += bytes;
        memcpy(write_ptr, new_sub, new_len);
        write_ptr += new_len;
        read_ptr = pos + old_len;
    }
    strcpy(write_ptr, read_ptr);

    return make_obj((Obj*)take_string(result, new_total_len));
}

static Value native_contains(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    Value needle = make_null();
    if (is_null(target)) {
        if (argc < 2) {
            runtime_error(line, "contains(container, item) expects 2 arguments!");
            return make_bool(false);
        }
        target = args[0];
        needle = args[1];
    } else {
        if (argc < 1) return make_bool(false);
        needle = args[0];
    }

    if (is_string(target) && is_string(needle)) {
        return make_bool(strstr(as_string(target)->chars, as_string(needle)->chars) != NULL);
    }
    if (is_list(target)) {
        ObjList* l = as_list(target);
        for (size_t i = 0; i < l->count; i++) {
            if (values_equal(l->items[i], needle)) return make_bool(true);
        }
        return make_bool(false);
    }
    if (is_map(target) && is_string(needle)) {
        Value dummy;
        return make_bool(map_get(as_map(target), as_string(needle)->chars, &dummy));
    }
    return make_bool(false);
}

static Value native_to_upper(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_string(args[0])) return make_null();
        target = args[0];
    } else if (!is_string(target)) return make_null();

    ObjString* str = as_string(target);
    char* res = (char*)malloc(str->length + 1);
    for (size_t i = 0; i < str->length; i++) {
        unsigned char c = (unsigned char)str->chars[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        res[i] = (char)c;
    }
    res[str->length] = '\0';
    return make_obj((Obj*)take_string(res, str->length));
}

static Value native_to_lower(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_string(args[0])) return make_null();
        target = args[0];
    } else if (!is_string(target)) return make_null();

    ObjString* str = as_string(target);
    char* res = (char*)malloc(str->length + 1);
    for (size_t i = 0; i < str->length; i++) {
        unsigned char c = (unsigned char)str->chars[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        res[i] = (char)c;
    }
    res[str->length] = '\0';
    return make_obj((Obj*)take_string(res, str->length));
}

static Value native_starts_with(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    Value prefix = make_null();
    if (is_null(target)) {
        if (argc < 2 || !is_string(args[0]) || !is_string(args[1])) return make_bool(false);
        target = args[0];
        prefix = args[1];
    } else {
        if (!is_string(target) || argc < 1 || !is_string(args[0])) return make_bool(false);
        prefix = args[0];
    }
    ObjString* s = as_string(target);
    ObjString* p = as_string(prefix);
    if (p->length > s->length) return make_bool(false);
    return make_bool(memcmp(s->chars, p->chars, p->length) == 0);
}

static Value native_ends_with(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    Value suffix = make_null();
    if (is_null(target)) {
        if (argc < 2 || !is_string(args[0]) || !is_string(args[1])) return make_bool(false);
        target = args[0];
        suffix = args[1];
    } else {
        if (!is_string(target) || argc < 1 || !is_string(args[0])) return make_bool(false);
        suffix = args[0];
    }
    ObjString* s = as_string(target);
    ObjString* p = as_string(suffix);
    if (p->length > s->length) return make_bool(false);
    return make_bool(memcmp(s->chars + (s->length - p->length), p->chars, p->length) == 0);
}

static Value native_slice(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    int start = 0;
    int end = -1;
    bool has_end = false;

    if (is_null(target)) {
        if (argc < 2) return make_null();
        target = args[0];
        if (is_number(args[1])) start = (int)as_number(args[1]);
        if (argc >= 3 && is_number(args[2])) {
            end = (int)as_number(args[2]);
            has_end = true;
        }
    } else {
        if (argc >= 1 && is_number(args[0])) start = (int)as_number(args[0]);
        if (argc >= 2 && is_number(args[1])) {
            end = (int)as_number(args[1]);
            has_end = true;
        }
    }

    if (is_string(target)) {
        ObjString* s = as_string(target);
        int len = (int)s->length;
        if (start < 0) start += len;
        if (start < 0) start = 0;
        if (start > len) start = len;
        if (!has_end) end = len;
        else if (end < 0) end += len;
        if (end < start) end = start;
        if (end > len) end = len;
        size_t res_len = end - start;
        char* buf = (char*)malloc(res_len + 1);
        memcpy(buf, s->chars + start, res_len);
        buf[res_len] = '\0';
        return make_obj((Obj*)take_string(buf, res_len));
    } else if (is_list(target)) {
        ObjList* l = as_list(target);
        int len = (int)l->count;
        if (start < 0) start += len;
        if (start < 0) start = 0;
        if (start > len) start = len;
        if (!has_end) end = len;
        else if (end < 0) end += len;
        if (end < start) end = start;
        if (end > len) end = len;
        ObjList* res = new_list();
        gc_push_root(make_obj((Obj*)res));
        for (int i = start; i < end; i++) {
            list_append(res, l->items[i]);
        }
        gc_pop_roots(1);
        return make_obj((Obj*)res);
    }
    return make_null();
}

static int compare_values(const void* a, const void* b) {
    Value va = *(const Value*)a;
    Value vb = *(const Value*)b;
    if (is_number(va) && is_number(vb)) {
        double diff = as_number(va) - as_number(vb);
        if (diff < 0) return -1;
        if (diff > 0) return 1;
        return 0;
    }
    if (is_string(va) && is_string(vb)) {
        return strcmp(as_string(va)->chars, as_string(vb)->chars);
    }
    return 0;
}

static Value native_sort(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_list(args[0])) {
            runtime_error(line, "sort() expects a list!");
            return make_null();
        }
        target = args[0];
    } else if (!is_list(target)) return make_null();

    ObjList* list = as_list(target);
    if (list->count > 1) {
        qsort(list->items, list->count, sizeof(Value), compare_values);
    }
    return target;
}

static Value native_reverse(Value self, size_t argc, Value* args, int line) {
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_list(args[0])) {
            runtime_error(line, "reverse() expects a list!");
            return make_null();
        }
        target = args[0];
    } else if (!is_list(target)) return make_null();

    ObjList* list = as_list(target);
    if (list->count > 1) {
        size_t i = 0, j = list->count - 1;
        while (i < j) {
            Value tmp = list->items[i];
            list->items[i] = list->items[j];
            list->items[j] = tmp;
            i++;
            j--;
        }
    }
    return target;
}

static Value native_keys(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_map(args[0])) return make_null();
        target = args[0];
    } else if (!is_map(target)) return make_null();

    ObjMap* map = as_map(target);
    ObjList* list = new_list();
    gc_push_root(make_obj((Obj*)list));
    for (size_t i = 0; i < map->count; i++) {
        list_append(list, make_obj((Obj*)copy_string(map->entries[i].key)));
    }
    gc_pop_roots(1);
    return make_obj((Obj*)list);
}

static Value native_values(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    if (is_null(target)) {
        if (argc < 1 || !is_map(args[0])) return make_null();
        target = args[0];
    } else if (!is_map(target)) return make_null();

    ObjMap* map = as_map(target);
    ObjList* list = new_list();
    gc_push_root(make_obj((Obj*)list));
    for (size_t i = 0; i < map->count; i++) {
        list_append(list, map->entries[i].value);
    }
    gc_pop_roots(1);
    return make_obj((Obj*)list);
}

static Value native_has_key(Value self, size_t argc, Value* args, int line) {
    (void)line;
    Value target = self;
    Value key = make_null();
    if (is_null(target)) {
        if (argc < 2 || !is_map(args[0]) || !is_string(args[1])) return make_bool(false);
        target = args[0];
        key = args[1];
    } else {
        if (!is_map(target) || argc < 1 || !is_string(args[0])) return make_bool(false);
        key = args[0];
    }
    Value val;
    return make_bool(map_get(as_map(target), as_string(key)->chars, &val));
}

// --- Console & System Functions ---
static Value native_clear(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)argc; (void)args; (void)line;
    printf("\033[H\033[2J");
    fflush(stdout);
    return make_null();
}

static Value native_import(Value self, size_t argc, Value* args, int line) {
    (void)self;
    if (argc < 1 || !is_string(args[0])) {
        runtime_error(line, "import() expects a string file path!");
        return make_bool(false);
    }
    const char* path = as_string(args[0])->chars;
    FILE* f = fopen(path, "rb");
    if (!f) {
        char try_ccl[512];
        snprintf(try_ccl, sizeof(try_ccl), "%s.ccl", path);
        f = fopen(try_ccl, "rb");
    }
    if (!f) {
        char try_ms[512];
        snprintf(try_ms, sizeof(try_ms), "%s.ms", path);
        f = fopen(try_ms, "rb");
    }
    if (!f) {
        runtime_error(line, "import: '%s' dosyası açılamadı!", path);
        return make_bool(false);
    }
    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return make_bool(false);
    }
    size_t read_bytes = fread(buffer, sizeof(char), size, f);
    buffer[read_bytes] = '\0';
    fclose(f);

    Parser parser;
    parser_init(&parser, buffer);
    size_t count = 0;
    Stmt** stmts = parse_program(&parser, &count);

    if (parser.had_error) {
        free(stmts);
        free(buffer);
        return make_bool(false);
    }

    Environment* env_to_use = g_active_env ? g_active_env : init_global_environment();
    EvalResult res = eval_statements(stmts, count, env_to_use);
    free(stmts);
    free(buffer);

    return make_bool(res.type != RES_ERROR);
}

void eval_set_args(Environment* env, int argc, char** argv, int script_idx) {
    ObjList* list = new_list();
    for (int i = script_idx; i < argc; i++) {
        list_append(list, make_obj((Obj*)copy_string(argv[i])));
    }
    Value val = make_obj((Obj*)list);
    env_define(env, "args", val);
    Value cicekli_val;
    if (env_get(env, "cicekli", &cicekli_val) && is_map(cicekli_val)) {
        map_set(as_map(cicekli_val), "args", val);
    }
    Value mete_val;
    if (env_get(env, "mete", &mete_val) && is_map(mete_val)) {
        map_set(as_map(mete_val), "args", val);
    }
}

static void define_native(Environment* env, ObjMap* cicekli_map, const char* name, NativeFn fn, int arity) {
    Value val = make_obj((Obj*)new_native(name, fn, arity));
    env_define(env, name, val);
    if (cicekli_map) {
        map_set(cicekli_map, name, val);
    }
}

Environment* init_global_environment(void) {
    Environment* env = new_environment(NULL);

    ObjMap* cicekli = new_map();
    map_set(cicekli, "versiyon", make_obj((Obj*)copy_string("1.0.0")));
    map_set(cicekli, "version", make_obj((Obj*)copy_string("1.0.0")));
    map_set(cicekli, "dil", make_obj((Obj*)copy_string("CicekliScript")));
    map_set(cicekli, "lang", make_obj((Obj*)copy_string("CicekliScript")));

    ObjMap* math_map = new_map();
    map_set(math_map, "pi", make_number(3.14159265358979323846));
    map_set(math_map, "e", make_number(2.71828182845904523536));

    ObjMap* file_map = new_map();

    // Standard I/O & Core
    define_native(env, cicekli, "println", native_println, -1);
    define_native(env, cicekli, "print", native_print, -1);
    define_native(env, cicekli, "input", native_input, -1);
    define_native(env, cicekli, "len", native_len, -1);
    define_native(env, cicekli, "range", native_range, -1);
    define_native(env, cicekli, "append", native_append, -1);
    define_native(env, cicekli, "pop", native_pop, -1);
    define_native(env, cicekli, "type", native_type, 1);
    define_native(env, cicekli, "str", native_str, 1);
    define_native(env, cicekli, "int", native_int, 1);
    define_native(env, cicekli, "float", native_float, 1);
    define_native(env, cicekli, "time", native_time, 0);
    define_native(env, cicekli, "sleep", native_sleep, 1);
    define_native(env, cicekli, "bekle", native_sleep, 1);
    define_native(env, cicekli, "exit", native_exit, -1);
    define_native(env, cicekli, "clear", native_clear, 0);
    define_native(env, cicekli, "cls", native_clear, 0);
    define_native(env, cicekli, "import", native_import, 1);
    define_native(env, cicekli, "window", native_mete_window, -1);

    // Math Functions (global + in math map + in cicekli map)
    #define DEF_MATH(name, fn, arity) \
        do { \
            define_native(env, cicekli, name, fn, arity); \
            map_set(math_map, name, make_obj((Obj*)new_native(name, fn, arity))); \
        } while (0)

    DEF_MATH("sqrt", native_sqrt, 1);
    DEF_MATH("pow", native_pow, 2);
    DEF_MATH("abs", native_abs, 1);
    DEF_MATH("round", native_round, 1);
    DEF_MATH("floor", native_floor, 1);
    DEF_MATH("ceil", native_ceil, 1);
    DEF_MATH("sin", native_sin, 1);
    DEF_MATH("cos", native_cos, 1);
    DEF_MATH("tan", native_tan, 1);
    DEF_MATH("log", native_log, 1);
    DEF_MATH("min", native_min, -1);
    DEF_MATH("max", native_max, -1);
    DEF_MATH("random", native_random, -1);
    #undef DEF_MATH

    // File I/O Functions (global + in file map + in cicekli map)
    #define DEF_FILE(name, fn, arity) \
        do { \
            define_native(env, cicekli, name, fn, arity); \
            map_set(file_map, name, make_obj((Obj*)new_native(name, fn, arity))); \
        } while (0)

    DEF_FILE("read_file", native_read_file, 1);
    DEF_FILE("write_file", native_write_file, 2);
    DEF_FILE("append_file", native_append_file, 2);
    DEF_FILE("file_exists", native_file_exists, 1);
    DEF_FILE("delete_file", native_delete_file, 1);
    // Short method names in file map: file.read, file.write, file.append, file.exists, file.delete
    map_set(file_map, "read", make_obj((Obj*)new_native("read", native_read_file, 1)));
    map_set(file_map, "write", make_obj((Obj*)new_native("write", native_write_file, 2)));
    map_set(file_map, "append", make_obj((Obj*)new_native("append", native_append_file, 2)));
    map_set(file_map, "exists", make_obj((Obj*)new_native("exists", native_file_exists, 1)));
    map_set(file_map, "delete", make_obj((Obj*)new_native("delete", native_delete_file, 1)));
    #undef DEF_FILE

    // String / List / Map Helpers
    define_native(env, cicekli, "split", native_split, -1);
    define_native(env, cicekli, "join", native_join, -1);
    define_native(env, cicekli, "trim", native_trim, -1);
    define_native(env, cicekli, "replace", native_replace, -1);
    define_native(env, cicekli, "contains", native_contains, -1);
    define_native(env, cicekli, "to_upper", native_to_upper, -1);
    define_native(env, cicekli, "to_lower", native_to_lower, -1);
    define_native(env, cicekli, "starts_with", native_starts_with, -1);
    define_native(env, cicekli, "ends_with", native_ends_with, -1);
    define_native(env, cicekli, "slice", native_slice, -1);
    define_native(env, cicekli, "sort", native_sort, -1);
    define_native(env, cicekli, "reverse", native_reverse, -1);
    define_native(env, cicekli, "keys", native_keys, -1);
    define_native(env, cicekli, "values", native_values, -1);
    define_native(env, cicekli, "has_key", native_has_key, -1);

    // Initial empty args
    ObjList* empty_args = new_list();
    env_define(env, "args", make_obj((Obj*)empty_args));
    map_set(cicekli, "args", make_obj((Obj*)empty_args));

    Value cicekli_obj = make_obj((Obj*)cicekli);
    env_define(env, "cicekli", cicekli_obj);
    env_define(env, "cicekliscript", cicekli_obj);
    env_define(env, "mete", cicekli_obj); // Geriye dönük uyumluluk
    env_define(env, "math", make_obj((Obj*)math_map));
    env_define(env, "file", make_obj((Obj*)file_map));
    return env;
}

Value eval_expression(Expr* expr, Environment* env, bool* had_error) {
    if (!expr || *had_error) return make_null();

    switch (expr->type) {
        case EXPR_LITERAL:
            return expr->as.literal;

        case EXPR_IDENTIFIER: {
            Value val;
            if (!env_get(env, expr->as.identifier, &val)) {
                runtime_error(expr->line, "Tanımsız değişken '%s'!", expr->as.identifier);
                *had_error = true;
                return make_null();
            }
            return val;
        }

        case EXPR_BINARY: {
            // Short-circuit logical operators
            if (expr->as.binary.op == TOKEN_AND || expr->as.binary.op == TOKEN_AMP_AMP) {
                Value left = eval_expression(expr->as.binary.left, env, had_error);
                if (*had_error) return make_null();
                if (!is_truthy(left)) return left;
                return eval_expression(expr->as.binary.right, env, had_error);
            }
            if (expr->as.binary.op == TOKEN_OR || expr->as.binary.op == TOKEN_PIPE_PIPE) {
                Value left = eval_expression(expr->as.binary.left, env, had_error);
                if (*had_error) return make_null();
                if (is_truthy(left)) return left;
                return eval_expression(expr->as.binary.right, env, had_error);
            }

            Value left = eval_expression(expr->as.binary.left, env, had_error);
            if (*had_error) return make_null();
            gc_push_root(left);

            Value right = eval_expression(expr->as.binary.right, env, had_error);
            gc_pop_roots(1);
            if (*had_error) return make_null();

            switch (expr->as.binary.op) {
                case TOKEN_PLUS: {
                    // String concatenation
                    if (is_string(left) || is_string(right)) {
                        char* sl = value_to_string(left);
                        char* sr = value_to_string(right);
                        size_t len_l = strlen(sl);
                        size_t len_r = strlen(sr);
                        char* res = (char*)malloc(len_l + len_r + 1);
                        memcpy(res, sl, len_l);
                        memcpy(res + len_l, sr, len_r);
                        res[len_l + len_r] = '\0';
                        free(sl);
                        free(sr);
                        return make_obj((Obj*)take_string(res, len_l + len_r));
                    }
                    // List concatenation
                    if (is_list(left) && is_list(right)) {
                        ObjList* l1 = as_list(left);
                        ObjList* l2 = as_list(right);
                        ObjList* res = new_list();
                        for (size_t i = 0; i < l1->count; i++) list_append(res, l1->items[i]);
                        for (size_t i = 0; i < l2->count; i++) list_append(res, l2->items[i]);
                        return make_obj((Obj*)res);
                    }
                    // Numbers
                    if (is_number(left) && is_number(right)) {
                        return make_number(as_number(left) + as_number(right));
                    }
                    runtime_error(expr->line, "'+' işlemi '%s' ve '%s' türleri arasında uygulanamaz!",
                                  value_type_name(left), value_type_name(right));
                    *had_error = true;
                    return make_null();
                }
                case TOKEN_MINUS:
                    if (is_number(left) && is_number(right)) return make_number(as_number(left) - as_number(right));
                    break;
                case TOKEN_STAR:
                    if (is_number(left) && is_number(right)) return make_number(as_number(left) * as_number(right));
                    // String repeat: "abc" * 3
                    if (is_string(left) && is_number(right)) {
                        ObjString* s = as_string(left);
                        int times = (int)as_number(right);
                        if (times <= 0) return make_obj((Obj*)copy_string(""));
                        char* res = (char*)malloc(s->length * times + 1);
                        for (int i = 0; i < times; i++) {
                            memcpy(res + i * s->length, s->chars, s->length);
                        }
                        res[s->length * times] = '\0';
                        return make_obj((Obj*)take_string(res, s->length * times));
                    }
                    break;
                case TOKEN_SLASH:
                    if (is_number(left) && is_number(right)) {
                        if (as_number(right) == 0.0) {
                            runtime_error(expr->line, "Sıfıra bölme hatası (Division by zero)!");
                            *had_error = true;
                            return make_null();
                        }
                        return make_number(as_number(left) / as_number(right));
                    }
                    break;
                case TOKEN_PERCENT:
                    if (is_number(left) && is_number(right)) {
                        if (as_number(right) == 0.0) {
                            runtime_error(expr->line, "Sıfıra göre mod alma hatası!");
                            *had_error = true;
                            return make_null();
                        }
                        return make_number(fmod(as_number(left), as_number(right)));
                    }
                    break;
                case TOKEN_EQUAL_EQUAL:
                    return make_bool(values_equal(left, right));
                case TOKEN_BANG_EQUAL:
                    return make_bool(!values_equal(left, right));
                case TOKEN_GREATER:
                    if (is_number(left) && is_number(right)) return make_bool(as_number(left) > as_number(right));
                    if (is_string(left) && is_string(right)) return make_bool(strcmp(as_string(left)->chars, as_string(right)->chars) > 0);
                    break;
                case TOKEN_GREATER_EQUAL:
                    if (is_number(left) && is_number(right)) return make_bool(as_number(left) >= as_number(right));
                    if (is_string(left) && is_string(right)) return make_bool(strcmp(as_string(left)->chars, as_string(right)->chars) >= 0);
                    break;
                case TOKEN_LESS:
                    if (is_number(left) && is_number(right)) return make_bool(as_number(left) < as_number(right));
                    if (is_string(left) && is_string(right)) return make_bool(strcmp(as_string(left)->chars, as_string(right)->chars) < 0);
                    break;
                case TOKEN_LESS_EQUAL:
                    if (is_number(left) && is_number(right)) return make_bool(as_number(left) <= as_number(right));
                    if (is_string(left) && is_string(right)) return make_bool(strcmp(as_string(left)->chars, as_string(right)->chars) <= 0);
                    break;
                default:
                    break;
            }

            runtime_error(expr->line, "Geçersiz ikili işlem (%s) türler: '%s' ve '%s'!",
                          token_type_name(expr->as.binary.op), value_type_name(left), value_type_name(right));
            *had_error = true;
            return make_null();
        }

        case EXPR_UNARY: {
            Value right = eval_expression(expr->as.unary.right, env, had_error);
            if (*had_error) return make_null();

            if (expr->as.unary.op == TOKEN_BANG || expr->as.unary.op == TOKEN_NOT) {
                return make_bool(!is_truthy(right));
            }
            if (expr->as.unary.op == TOKEN_MINUS) {
                if (is_number(right)) {
                    return make_number(-as_number(right));
                }
                runtime_error(expr->line, "'-' işlemi sayı bekler!");
                *had_error = true;
                return make_null();
            }
            break;
        }

        case EXPR_CALL: {
            Value callee = eval_expression(expr->as.call.callee, env, had_error);
            if (*had_error) return make_null();
            gc_push_root(callee);

            size_t argc = expr->as.call.arg_count;
            Value* args = (Value*)malloc(sizeof(Value) * (argc > 0 ? argc : 1));

            for (size_t i = 0; i < argc; i++) {
                args[i] = eval_expression(expr->as.call.args[i], env, had_error);
                if (*had_error) {
                    free(args);
                    gc_pop_roots(1);
                    return make_null();
                }
                gc_push_root(args[i]);
            }

            Value result = make_null();
            Value receiver = make_null();

            if (is_bound_method(callee)) {
                ObjBoundMethod* bm = as_bound_method(callee);
                receiver = bm->receiver;
                callee = bm->method;
            }

            if (is_native(callee)) {
                ObjNative* nat = as_native(callee);
                if (nat->arity != -1 && (size_t)nat->arity != argc) {
                    runtime_error(expr->line, "Yerel fonksiyon '%s' %d argüman bekler, %zu verildi!",
                                  nat->name, nat->arity, argc);
                    *had_error = true;
                } else {
                    result = nat->fn(receiver, argc, args, expr->line);
                }
            } else if (is_function(callee)) {
                ObjFunction* fn = as_function(callee);
                if (fn->arity != argc) {
                    runtime_error(expr->line, "Fonksiyon '%s' %zu parametre bekler, %zu verildi!",
                                  fn->name ? fn->name : "anonim", fn->arity, argc);
                    *had_error = true;
                } else {
                    Environment* fn_env = new_environment(fn->closure);
                    if (!is_null(receiver)) {
                        env_define(fn_env, "this", receiver);
                        env_define(fn_env, "self", receiver);
                    }
                    for (size_t i = 0; i < argc; i++) {
                        env_define(fn_env, fn->params[i], args[i]);
                    }
                    EvalResult res = eval_statement(fn->body, fn_env);
                    if (res.type == RES_RETURN) {
                        result = res.value;
                    } else if (res.type == RES_ERROR) {
                        *had_error = true;
                    } else {
                        result = make_null();
                    }
                }
            } else {
                runtime_error(expr->line, "Yalnızca fonksiyonlar çağrılabilir! ('%s' çağrılamaz)",
                              value_type_name(callee));
                *had_error = true;
            }

            gc_pop_roots(argc + 1); // args and callee
            free(args);
            return result;
        }

        case EXPR_MEMBER: {
            Value obj = eval_expression(expr->as.member.object, env, had_error);
            if (*had_error) return make_null();

            if (is_window(obj)) {
                Value m = gui_window_get_method(obj, expr->as.member.member);
                if (!is_null(m)) {
                    return make_obj((Obj*)new_bound_method(obj, m));
                }
                Value p = gui_window_get_property(obj, expr->as.member.member);
                if (!is_null(p)) {
                    return p;
                }
                runtime_error(expr->line, "Pencerede '%s' özelliği veya metodu bulunamadı!", expr->as.member.member);
                *had_error = true;
                return make_null();
            }

            if (is_list(obj)) {
                const char* m = expr->as.member.member;
                if (strcmp(m, "append") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("append", native_append, 1))));
                if (strcmp(m, "pop") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("pop", native_pop, -1))));
                if (strcmp(m, "len") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("len", native_len, 0))));
                if (strcmp(m, "sort") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("sort", native_sort, 0))));
                if (strcmp(m, "reverse") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("reverse", native_reverse, 0))));
                if (strcmp(m, "join") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("join", native_join, -1))));
                if (strcmp(m, "contains") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("contains", native_contains, 1))));
                if (strcmp(m, "slice") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("slice", native_slice, -1))));
                if (strcmp(m, "length") == 0) return make_number((double)as_list(obj)->count);
            }

            if (is_string(obj)) {
                const char* m = expr->as.member.member;
                if (strcmp(m, "len") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("len", native_len, 0))));
                if (strcmp(m, "split") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("split", native_split, -1))));
                if (strcmp(m, "trim") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("trim", native_trim, 0))));
                if (strcmp(m, "replace") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("replace", native_replace, 2))));
                if (strcmp(m, "contains") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("contains", native_contains, 1))));
                if (strcmp(m, "to_upper") == 0 || strcmp(m, "upper") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("to_upper", native_to_upper, 0))));
                if (strcmp(m, "to_lower") == 0 || strcmp(m, "lower") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("to_lower", native_to_lower, 0))));
                if (strcmp(m, "starts_with") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("starts_with", native_starts_with, 1))));
                if (strcmp(m, "ends_with") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("ends_with", native_ends_with, 1))));
                if (strcmp(m, "slice") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("slice", native_slice, -1))));
                if (strcmp(m, "length") == 0) return make_number((double)as_string(obj)->length);
            }

            if (is_map(obj)) {
                ObjMap* map = as_map(obj);
                const char* m = expr->as.member.member;
                if (strcmp(m, "keys") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("keys", native_keys, 0))));
                if (strcmp(m, "values") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("values", native_values, 0))));
                if (strcmp(m, "has_key") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("has_key", native_has_key, 1))));
                if (strcmp(m, "len") == 0) return make_obj((Obj*)new_bound_method(obj, make_obj((Obj*)new_native("len", native_len, 0))));
                Value val;
                if (map_get(map, m, &val)) {
                    if (is_native(val) || is_function(val)) {
                        return make_obj((Obj*)new_bound_method(obj, val));
                    }
                    return val;
                }
                runtime_error(expr->line, "Haritada '%s' özelliği bulunamadı!", m);
                *had_error = true;
                return make_null();
            }

            runtime_error(expr->line, "'%s' türünde özellik erişimi yapılamaz!", value_type_name(obj));
            *had_error = true;
            return make_null();
        }

        case EXPR_INDEX: {
            Value obj = eval_expression(expr->as.index.object, env, had_error);
            if (*had_error) return make_null();
            gc_push_root(obj);

            Value idx = eval_expression(expr->as.index.index, env, had_error);
            gc_pop_roots(1);
            if (*had_error) return make_null();

            if (is_list(obj)) {
                if (!is_number(idx)) {
                    runtime_error(expr->line, "Liste indeksi sayı olmalıdır!");
                    *had_error = true;
                    return make_null();
                }
                Value item;
                if (!list_get(as_list(obj), (int64_t)as_number(idx), &item)) {
                    runtime_error(expr->line, "Liste indeksi aralık dışı (out of range)!");
                    *had_error = true;
                    return make_null();
                }
                return item;
            }

            if (is_string(obj)) {
                if (!is_number(idx)) {
                    runtime_error(expr->line, "Metin indeksi sayı olmalıdır!");
                    *had_error = true;
                    return make_null();
                }
                ObjString* str = as_string(obj);
                int64_t i = (int64_t)as_number(idx);
                if (i < 0) i += (int64_t)str->length;
                if (i < 0 || (size_t)i >= str->length) {
                    runtime_error(expr->line, "Metin indeksi aralık dışı!");
                    *had_error = true;
                    return make_null();
                }
                char ch[2] = { str->chars[i], '\0' };
                return make_obj((Obj*)copy_string(ch));
            }

            if (is_map(obj)) {
                if (!is_string(idx)) {
                    runtime_error(expr->line, "Harita (map) anahtarı metin olmalıdır!");
                    *had_error = true;
                    return make_null();
                }
                Value item;
                if (!map_get(as_map(obj), as_string(idx)->chars, &item)) {
                    runtime_error(expr->line, "Haritada '%s' anahtarı bulunamadı!", as_string(idx)->chars);
                    *had_error = true;
                    return make_null();
                }
                return item;
            }

            runtime_error(expr->line, "'%s' türü indekslenemez!", value_type_name(obj));
            *had_error = true;
            return make_null();
        }

        case EXPR_LIST: {
            ObjList* list = new_list();
            gc_push_root(make_obj((Obj*)list));
            for (size_t i = 0; i < expr->as.list.count; i++) {
                Value elem = eval_expression(expr->as.list.elements[i], env, had_error);
                if (*had_error) {
                    gc_pop_roots(1);
                    return make_null();
                }
                list_append(list, elem);
            }
            gc_pop_roots(1);
            return make_obj((Obj*)list);
        }

        case EXPR_MAP: {
            ObjMap* map = new_map();
            gc_push_root(make_obj((Obj*)map));
            for (size_t i = 0; i < expr->as.map.count; i++) {
                Value val = eval_expression(expr->as.map.entries[i].value, env, had_error);
                if (*had_error) {
                    gc_pop_roots(1);
                    return make_null();
                }
                map_set(map, expr->as.map.entries[i].key, val);
            }
            gc_pop_roots(1);
            return make_obj((Obj*)map);
        }

        case EXPR_ASSIGN: {
            Value val = eval_expression(expr->as.assign.value, env, had_error);
            if (*had_error) return make_null();

            if (expr->as.assign.op != TOKEN_EQUAL) {
                Value current;
                if (!env_get(env, expr->as.assign.name, &current)) {
                    runtime_error(expr->line, "Tanımsız değişken '%s'!", expr->as.assign.name);
                    *had_error = true;
                    return make_null();
                }
                if (expr->as.assign.op == TOKEN_PLUS_EQUAL) {
                    if (is_number(current) && is_number(val)) {
                        val = make_number(as_number(current) + as_number(val));
                    } else if (is_string(current) || is_string(val)) {
                        char* sl = value_to_string(current);
                        char* sr = value_to_string(val);
                        size_t len_l = strlen(sl);
                        size_t len_r = strlen(sr);
                        char* res = (char*)malloc(len_l + len_r + 1);
                        memcpy(res, sl, len_l);
                        memcpy(res + len_l, sr, len_r);
                        res[len_l + len_r] = '\0';
                        free(sl);
                        free(sr);
                        val = make_obj((Obj*)take_string(res, len_l + len_r));
                    }
                } else if (expr->as.assign.op == TOKEN_MINUS_EQUAL && is_number(current) && is_number(val)) {
                    val = make_number(as_number(current) - as_number(val));
                } else if (expr->as.assign.op == TOKEN_STAR_EQUAL && is_number(current) && is_number(val)) {
                    val = make_number(as_number(current) * as_number(val));
                } else if (expr->as.assign.op == TOKEN_SLASH_EQUAL && is_number(current) && is_number(val)) {
                    if (as_number(val) == 0.0) {
                        runtime_error(expr->line, "Sıfıra bölme hatası!");
                        *had_error = true;
                        return make_null();
                    }
                    val = make_number(as_number(current) / as_number(val));
                }
            }

            env_set(env, expr->as.assign.name, val);
            return val;
        }

        case EXPR_MEMBER_ASSIGN: {
            Value obj = eval_expression(expr->as.member_assign.object, env, had_error);
            if (*had_error) return make_null();
            gc_push_root(obj);

            Value val = eval_expression(expr->as.member_assign.value, env, had_error);
            gc_pop_roots(1);
            if (*had_error) return make_null();

            if (is_map(obj)) {
                map_set(as_map(obj), expr->as.member_assign.member, val);
                return val;
            }

            runtime_error(expr->line, "'%s' türünün alanına atama yapılamaz!", value_type_name(obj));
            *had_error = true;
            return make_null();
        }

        case EXPR_INDEX_ASSIGN: {
            Value obj = eval_expression(expr->as.index_assign.object, env, had_error);
            if (*had_error) return make_null();
            gc_push_root(obj);

            Value idx = eval_expression(expr->as.index_assign.index, env, had_error);
            gc_push_root(idx);

            Value val = eval_expression(expr->as.index_assign.value, env, had_error);
            gc_pop_roots(2);
            if (*had_error) return make_null();

            if (is_list(obj)) {
                if (!is_number(idx)) {
                    runtime_error(expr->line, "Liste indeksi sayı olmalıdır!");
                    *had_error = true;
                    return make_null();
                }
                if (!list_set(as_list(obj), (int64_t)as_number(idx), val)) {
                    runtime_error(expr->line, "Liste indeksi aralık dışı!");
                    *had_error = true;
                    return make_null();
                }
                return val;
            }

            if (is_map(obj)) {
                if (!is_string(idx)) {
                    runtime_error(expr->line, "Harita indeksi metin olmalıdır!");
                    *had_error = true;
                    return make_null();
                }
                map_set(as_map(obj), as_string(idx)->chars, val);
                return val;
            }

            runtime_error(expr->line, "'%s' türüne indeks ataması yapılamaz!", value_type_name(obj));
            *had_error = true;
            return make_null();
        }

        case EXPR_FUNCTION: {
            ObjFunction* fn = new_function(NULL, expr->as.function.params, expr->as.function.arity,
                                           expr->as.function.body, env);
            return make_obj((Obj*)fn);
        }
    }

    return make_null();
}

EvalResult eval_statement(Stmt* stmt, Environment* env) {
    EvalResult res;
    res.type = RES_OK;
    res.value = make_null();

    if (!stmt) return res;

    switch (stmt->type) {
        case STMT_EXPR: {
            bool error = false;
            eval_expression(stmt->as.expr, env, &error);
            if (error) res.type = RES_ERROR;
            return res;
        }

        case STMT_BLOCK: {
            Environment* block_env = new_environment(env);
            for (size_t i = 0; i < stmt->as.block.count; i++) {
                res = eval_statement(stmt->as.block.stmts[i], block_env);
                if (res.type != RES_OK) {
                    return res;
                }
            }
            return res;
        }

        case STMT_IF: {
            bool error = false;
            Value cond = eval_expression(stmt->as.if_stmt.condition, env, &error);
            if (error) { res.type = RES_ERROR; return res; }

            if (is_truthy(cond)) {
                return eval_statement(stmt->as.if_stmt.then_branch, env);
            }

            for (size_t i = 0; i < stmt->as.if_stmt.elif_count; i++) {
                Value elif_cond = eval_expression(stmt->as.if_stmt.elifs[i].condition, env, &error);
                if (error) { res.type = RES_ERROR; return res; }
                if (is_truthy(elif_cond)) {
                    return eval_statement(stmt->as.if_stmt.elifs[i].body, env);
                }
            }

            if (stmt->as.if_stmt.else_branch) {
                return eval_statement(stmt->as.if_stmt.else_branch, env);
            }
            return res;
        }

        case STMT_WHILE: {
            for (;;) {
                bool error = false;
                Value cond = eval_expression(stmt->as.while_stmt.condition, env, &error);
                if (error) { res.type = RES_ERROR; return res; }
                if (!is_truthy(cond)) break;

                res = eval_statement(stmt->as.while_stmt.body, env);
                if (res.type == RES_BREAK) {
                    res.type = RES_OK;
                    break;
                }
                if (res.type == RES_RETURN || res.type == RES_ERROR) {
                    return res;
                }
                if (res.type == RES_CONTINUE) {
                    res.type = RES_OK;
                    continue;
                }
            }
            return res;
        }

        case STMT_FOR_IN: {
            bool error = false;
            Value iter = eval_expression(stmt->as.for_in.iterable, env, &error);
            if (error) { res.type = RES_ERROR; return res; }

            if (is_list(iter)) {
                ObjList* list = as_list(iter);
                for (size_t i = 0; i < list->count; i++) {
                    Environment* loop_env = new_environment(env);
                    env_define(loop_env, stmt->as.for_in.var_name, list->items[i]);
                    res = eval_statement(stmt->as.for_in.body, loop_env);
                    if (res.type == RES_BREAK) {
                        res.type = RES_OK;
                        break;
                    }
                    if (res.type == RES_RETURN || res.type == RES_ERROR) {
                        return res;
                    }
                    if (res.type == RES_CONTINUE) {
                        res.type = RES_OK;
                        continue;
                    }
                }
            } else if (is_string(iter)) {
                ObjString* str = as_string(iter);
                for (size_t i = 0; i < str->length; i++) {
                    char ch[2] = { str->chars[i], '\0' };
                    Value ch_val = make_obj((Obj*)copy_string(ch));
                    Environment* loop_env = new_environment(env);
                    env_define(loop_env, stmt->as.for_in.var_name, ch_val);
                    res = eval_statement(stmt->as.for_in.body, loop_env);
                    if (res.type == RES_BREAK) {
                        res.type = RES_OK;
                        break;
                    }
                    if (res.type == RES_RETURN || res.type == RES_ERROR) {
                        return res;
                    }
                    if (res.type == RES_CONTINUE) {
                        res.type = RES_OK;
                        continue;
                    }
                }
            } else {
                runtime_error(stmt->line, "'%s' türü üzerinde for döngüsü kurulamaz!", value_type_name(iter));
                res.type = RES_ERROR;
            }
            return res;
        }

        case STMT_FOR_C: {
            Environment* loop_env = new_environment(env);
            if (stmt->as.for_c.init) {
                res = eval_statement(stmt->as.for_c.init, loop_env);
                if (res.type != RES_OK) return res;
            }

            for (;;) {
                if (stmt->as.for_c.cond) {
                    bool error = false;
                    Value cond = eval_expression(stmt->as.for_c.cond, loop_env, &error);
                    if (error) { res.type = RES_ERROR; return res; }
                    if (!is_truthy(cond)) break;
                }

                res = eval_statement(stmt->as.for_c.body, loop_env);
                if (res.type == RES_BREAK) {
                    res.type = RES_OK;
                    break;
                }
                if (res.type == RES_RETURN || res.type == RES_ERROR) {
                    return res;
                }

                if (stmt->as.for_c.step) {
                    bool error = false;
                    eval_expression(stmt->as.for_c.step, loop_env, &error);
                    if (error) { res.type = RES_ERROR; return res; }
                }
            }
            return res;
        }

        case STMT_FUNCTION: {
            ObjFunction* fn = new_function(stmt->as.function.name, stmt->as.function.params,
                                           stmt->as.function.arity, stmt->as.function.body, env);
            env_define(env, stmt->as.function.name, make_obj((Obj*)fn));
            return res;
        }

        case STMT_RETURN: {
            res.type = RES_RETURN;
            if (stmt->as.return_expr) {
                bool error = false;
                res.value = eval_expression(stmt->as.return_expr, env, &error);
                if (error) res.type = RES_ERROR;
            } else {
                res.value = make_null();
            }
            return res;
        }

        case STMT_BREAK:
            res.type = RES_BREAK;
            return res;

        case STMT_CONTINUE:
            res.type = RES_CONTINUE;
            return res;
    }

    return res;
}

EvalResult eval_statements(Stmt** stmts, size_t count, Environment* env) {
    Environment* prev_env = g_active_env;
    g_active_env = env;
    EvalResult res;
    res.type = RES_OK;
    res.value = make_null();

    for (size_t i = 0; i < count; i++) {
        res = eval_statement(stmts[i], env);
        if (res.type != RES_OK) {
            g_active_env = prev_env;
            return res;
        }
    }

    g_active_env = prev_env;
    return res;
}
