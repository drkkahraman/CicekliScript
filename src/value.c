#include "value.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

Value make_null(void) {
    Value v;
    v.type = VAL_NULL;
    v.as.number = 0;
    return v;
}

Value make_bool(bool b) {
    Value v;
    v.type = VAL_BOOL;
    v.as.boolean = b;
    return v;
}

Value make_number(double n) {
    Value v;
    v.type = VAL_NUMBER;
    v.as.number = n;
    return v;
}

Value make_obj(Obj* o) {
    Value v;
    v.type = VAL_OBJ;
    v.as.obj = o;
    return v;
}

static uint32_t hash_string(const char* key, size_t length) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* new_string(const char* chars, size_t length) {
    char* heap_chars = (char*)malloc(length + 1);
    if (!heap_chars) exit(1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return take_string(heap_chars, length);
}

ObjString* copy_string(const char* chars) {
    return new_string(chars, strlen(chars));
}

ObjString* take_string(char* chars, size_t length) {
    ObjString* str = (ObjString*)gc_allocate(sizeof(ObjString), OBJ_STRING);
    str->chars = chars;
    str->length = length;
    str->hash = hash_string(chars, length);
    return str;
}

ObjList* new_list(void) {
    ObjList* list = (ObjList*)gc_allocate(sizeof(ObjList), OBJ_LIST);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

void list_append(ObjList* list, Value v) {
    if (list->count >= list->capacity) {
        size_t old_cap = list->capacity;
        list->capacity = old_cap < 8 ? 8 : old_cap * 2;
        list->items = (Value*)realloc(list->items, sizeof(Value) * list->capacity);
        if (!list->items) exit(1);
    }
    list->items[list->count++] = v;
}

static int64_t normalize_index(int64_t index, size_t length) {
    if (index < 0) {
        index += (int64_t)length;
    }
    return index;
}

bool list_set(ObjList* list, int64_t index, Value v) {
    index = normalize_index(index, list->count);
    if (index < 0 || (size_t)index >= list->count) {
        return false;
    }
    list->items[index] = v;
    return true;
}

bool list_get(ObjList* list, int64_t index, Value* out) {
    index = normalize_index(index, list->count);
    if (index < 0 || (size_t)index >= list->count) {
        return false;
    }
    *out = list->items[index];
    return true;
}

Value list_pop(ObjList* list, int64_t index) {
    if (list->count == 0) return make_null();
    index = normalize_index(index, list->count);
    if (index < 0 || (size_t)index >= list->count) {
        return make_null();
    }
    Value val = list->items[index];
    for (size_t i = (size_t)index; i < list->count - 1; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->count--;
    return val;
}

ObjMap* new_map(void) {
    ObjMap* map = (ObjMap*)gc_allocate(sizeof(ObjMap), OBJ_MAP);
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
    return map;
}

void map_set(ObjMap* map, const char* key, Value value) {
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->entries[i].key, key) == 0) {
            map->entries[i].value = value;
            return;
        }
    }
    if (map->count >= map->capacity) {
        size_t old_cap = map->capacity;
        map->capacity = old_cap < 8 ? 8 : old_cap * 2;
        map->entries = (MapEntry*)realloc(map->entries, sizeof(MapEntry) * map->capacity);
        if (!map->entries) exit(1);
    }
    map->entries[map->count].key = strdup(key);
    map->entries[map->count].value = value;
    map->count++;
}

bool map_get(ObjMap* map, const char* key, Value* out) {
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->entries[i].key, key) == 0) {
            *out = map->entries[i].value;
            return true;
        }
    }
    return false;
}

ObjFunction* new_function(const char* name, char** params, size_t arity, struct Stmt* body, Environment* closure) {
    ObjFunction* fn = (ObjFunction*)gc_allocate(sizeof(ObjFunction), OBJ_FUNCTION);
    fn->name = name ? strdup(name) : NULL;
    fn->arity = arity;
    fn->params = (char**)malloc(sizeof(char*) * (arity > 0 ? arity : 1));
    for (size_t i = 0; i < arity; i++) {
        fn->params[i] = strdup(params[i]);
    }
    fn->body = body;
    fn->closure = closure;
    return fn;
}

ObjNative* new_native(const char* name, NativeFn fn, int arity) {
    ObjNative* nat = (ObjNative*)gc_allocate(sizeof(ObjNative), OBJ_NATIVE);
    nat->name = strdup(name);
    nat->fn = fn;
    nat->arity = arity;
    return nat;
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_NULL: return true;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_NUMBER: return a.as.number == b.as.number;
        case VAL_OBJ: {
            if (a.as.obj == b.as.obj) return true;
            if (a.as.obj->type != b.as.obj->type) return false;
            if (a.as.obj->type == OBJ_STRING) {
                ObjString* sa = as_string(a);
                ObjString* sb = as_string(b);
                return sa->length == sb->length && memcmp(sa->chars, sb->chars, sa->length) == 0;
            }
            return false;
        }
    }
    return false;
}

bool is_truthy(Value v) {
    switch (v.type) {
        case VAL_NULL: return false;
        case VAL_BOOL: return v.as.boolean;
        case VAL_NUMBER: return v.as.number != 0.0;
        case VAL_OBJ: {
            if (v.as.obj->type == OBJ_STRING) {
                return as_string(v)->length > 0;
            }
            if (v.as.obj->type == OBJ_LIST) {
                return as_list(v)->count > 0;
            }
            return true;
        }
    }
    return false;
}

const char* value_type_name(Value v) {
    switch (v.type) {
        case VAL_NULL: return "null";
        case VAL_BOOL: return "bool";
        case VAL_NUMBER: return "number";
        case VAL_OBJ: {
            switch (v.as.obj->type) {
                case OBJ_STRING: return "string";
                case OBJ_LIST: return "list";
                case OBJ_MAP: return "map";
                case OBJ_FUNCTION: return "function";
                case OBJ_NATIVE: return "native_fn";
                case OBJ_ENV: return "env";
            }
        }
    }
    return "unknown";
}

char* value_to_string(Value v) {
    char buffer[256];
    switch (v.type) {
        case VAL_NULL:
            return strdup("null");
        case VAL_BOOL:
            return strdup(v.as.boolean ? "true" : "false");
        case VAL_NUMBER: {
            double n = v.as.number;
            if (floor(n) == n && !isinf(n) && !isnan(n)) {
                snprintf(buffer, sizeof(buffer), "%.0f", n);
            } else {
                snprintf(buffer, sizeof(buffer), "%g", n);
            }
            return strdup(buffer);
        }
        case VAL_OBJ: {
            switch (v.as.obj->type) {
                case OBJ_STRING:
                    return strdup(as_string(v)->chars);
                case OBJ_LIST: {
                    ObjList* list = as_list(v);
                    size_t cap = 64;
                    char* out = (char*)malloc(cap);
                    strcpy(out, "[");
                    size_t len = 1;
                    for (size_t i = 0; i < list->count; i++) {
                        char* s = value_to_string(list->items[i]);
                        size_t slen = strlen(s);
                        if (len + slen + 4 >= cap) {
                            cap = (len + slen + 4) * 2;
                            out = (char*)realloc(out, cap);
                        }
                        if (i > 0) {
                            strcat(out, ", ");
                            len += 2;
                        }
                        if (is_string(list->items[i])) {
                            strcat(out, "\"");
                            strcat(out, s);
                            strcat(out, "\"");
                            len += slen + 2;
                        } else {
                            strcat(out, s);
                            len += slen;
                        }
                        free(s);
                    }
                    strcat(out, "]");
                    return out;
                }
                case OBJ_MAP: {
                    ObjMap* map = as_map(v);
                    size_t cap = 64;
                    char* out = (char*)malloc(cap);
                    strcpy(out, "{");
                    size_t len = 1;
                    for (size_t i = 0; i < map->count; i++) {
                        char* val_str = value_to_string(map->entries[i].value);
                        size_t klen = strlen(map->entries[i].key);
                        size_t vlen = strlen(val_str);
                        size_t needed = len + klen + vlen + 10;
                        if (needed >= cap) {
                            cap = needed * 2;
                            out = (char*)realloc(out, cap);
                        }
                        if (i > 0) {
                            strcat(out, ", ");
                            len += 2;
                        }
                        strcat(out, "\"");
                        strcat(out, map->entries[i].key);
                        strcat(out, "\": ");
                        len += klen + 4;
                        if (is_string(map->entries[i].value)) {
                            strcat(out, "\"");
                            strcat(out, val_str);
                            strcat(out, "\"");
                            len += vlen + 2;
                        } else {
                            strcat(out, val_str);
                            len += vlen;
                        }
                        free(val_str);
                    }
                    strcat(out, "}");
                    return out;
                }
                case OBJ_FUNCTION: {
                    ObjFunction* fn = as_function(v);
                    snprintf(buffer, sizeof(buffer), "<mete.function %s>", fn->name ? fn->name : "anonim");
                    return strdup(buffer);
                }
                case OBJ_NATIVE: {
                    ObjNative* nat = as_native(v);
                    snprintf(buffer, sizeof(buffer), "<mete.native %s>", nat->name);
                    return strdup(buffer);
                }
                default:
                    return strdup("<obj>");
            }
        }
    }
    return strdup("<unknown>");
}

void print_value(Value v) {
    char* s = value_to_string(v);
    fputs(s, stdout);
    free(s);
}
