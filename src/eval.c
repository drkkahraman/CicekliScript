#include "eval.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

static void runtime_error(int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "MeteScript Çalışma Zamanı Hatası (Satır %d): ", line);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

// Built-in functions
static Value native_println(size_t arg_count, Value* args, int line) {
    (void)line;
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

static Value native_print(size_t arg_count, Value* args, int line) {
    (void)line;
    for (size_t i = 0; i < arg_count; i++) {
        print_value(args[i]);
        if (i + 1 < arg_count) {
            putchar(' ');
        }
    }
    fflush(stdout);
    return make_null();
}

static Value native_input(size_t arg_count, Value* args, int line) {
    (void)line;
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

static Value native_len(size_t arg_count, Value* args, int line) {
    if (arg_count != 1) {
        runtime_error(line, "len() tam olarak 1 argüman bekler!");
        return make_null();
    }
    Value v = args[0];
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

static Value native_range(size_t arg_count, Value* args, int line) {
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

static Value native_append(size_t arg_count, Value* args, int line) {
    if (arg_count != 2) {
        runtime_error(line, "append() tam olarak 2 argüman (liste, eleman) bekler!");
        return make_null();
    }
    if (!is_list(args[0])) {
        runtime_error(line, "append() ilk argüman olarak liste bekler!");
        return make_null();
    }
    ObjList* list = as_list(args[0]);
    list_append(list, args[1]);
    return args[0];
}

static Value native_pop(size_t arg_count, Value* args, int line) {
    if (arg_count < 1 || arg_count > 2) {
        runtime_error(line, "pop() 1 veya 2 argüman bekler!");
        return make_null();
    }
    if (!is_list(args[0])) {
        runtime_error(line, "pop() ilk argüman olarak liste bekler!");
        return make_null();
    }
    ObjList* list = as_list(args[0]);
    int64_t idx = -1;
    if (arg_count == 2) {
        if (!is_number(args[1])) {
            runtime_error(line, "pop() indeks olarak sayı bekler!");
            return make_null();
        }
        idx = (int64_t)as_number(args[1]);
    }
    return list_pop(list, idx);
}

static Value native_type(size_t arg_count, Value* args, int line) {
    if (arg_count != 1) {
        runtime_error(line, "type() tam olarak 1 argüman bekler!");
        return make_null();
    }
    const char* tname = value_type_name(args[0]);
    return make_obj((Obj*)copy_string(tname));
}

static Value native_str(size_t arg_count, Value* args, int line) {
    if (arg_count != 1) {
        runtime_error(line, "str() tam olarak 1 argüman bekler!");
        return make_null();
    }
    char* s = value_to_string(args[0]);
    ObjString* str_obj = copy_string(s);
    free(s);
    return make_obj((Obj*)str_obj);
}

static Value native_int(size_t arg_count, Value* args, int line) {
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

static Value native_float(size_t arg_count, Value* args, int line) {
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

static Value native_time(size_t arg_count, Value* args, int line) {
    (void)arg_count; (void)args; (void)line;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    double sec = (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
    return make_number(sec);
}

static Value native_exit(size_t arg_count, Value* args, int line) {
    (void)line;
    int code = 0;
    if (arg_count > 0 && is_number(args[0])) {
        code = (int)as_number(args[0]);
    }
    exit(code);
    return make_null();
}

static void define_native(Environment* env, ObjMap* mete_map, const char* name, NativeFn fn, int arity) {
    Value val = make_obj((Obj*)new_native(name, fn, arity));
    env_define(env, name, val);
    if (mete_map) {
        map_set(mete_map, name, val);
    }
}

Environment* init_global_environment(void) {
    Environment* env = new_environment(NULL);

    ObjMap* mete = new_map();
    map_set(mete, "versiyon", make_obj((Obj*)copy_string("1.0.0")));
    map_set(mete, "dil", make_obj((Obj*)copy_string("MeteScript")));

    define_native(env, mete, "println", native_println, -1);
    define_native(env, mete, "print", native_print, -1);
    define_native(env, mete, "input", native_input, -1);
    define_native(env, mete, "len", native_len, 1);
    define_native(env, mete, "range", native_range, -1);
    define_native(env, mete, "append", native_append, 2);
    define_native(env, mete, "pop", native_pop, -1);
    define_native(env, mete, "type", native_type, 1);
    define_native(env, mete, "str", native_str, 1);
    define_native(env, mete, "int", native_int, 1);
    define_native(env, mete, "float", native_float, 1);
    define_native(env, mete, "time", native_time, 0);
    define_native(env, mete, "exit", native_exit, -1);

    env_define(env, "mete", make_obj((Obj*)mete));
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

            if (is_native(callee)) {
                ObjNative* nat = as_native(callee);
                if (nat->arity != -1 && (size_t)nat->arity != argc) {
                    runtime_error(expr->line, "Yerel fonksiyon '%s' %d argüman bekler, %zu verildi!",
                                  nat->name, nat->arity, argc);
                    *had_error = true;
                } else {
                    result = nat->fn(argc, args, expr->line);
                }
            } else if (is_function(callee)) {
                ObjFunction* fn = as_function(callee);
                if (fn->arity != argc) {
                    runtime_error(expr->line, "Fonksiyon '%s' %zu parametre bekler, %zu verildi!",
                                  fn->name ? fn->name : "anonim", fn->arity, argc);
                    *had_error = true;
                } else {
                    Environment* fn_env = new_environment(fn->closure);
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

            if (is_map(obj)) {
                ObjMap* map = as_map(obj);
                Value val;
                if (map_get(map, expr->as.member.member, &val)) {
                    return val;
                }
                runtime_error(expr->line, "Haritada '%s' özelliği bulunamadı!", expr->as.member.member);
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
    EvalResult res;
    res.type = RES_OK;
    res.value = make_null();

    for (size_t i = 0; i < count; i++) {
        res = eval_statement(stmts[i], env);
        if (res.type != RES_OK) {
            return res;
        }
    }

    return res;
}
