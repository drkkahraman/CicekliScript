#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Obj* all_objects = NULL;
static size_t bytes_allocated = 0;
static size_t next_gc = 1024 * 1024; // 1MB initial threshold

#define ROOT_STACK_MAX 1024
static Value root_stack[ROOT_STACK_MAX];
static size_t root_count = 0;

void gc_init(void) {
    all_objects = NULL;
    bytes_allocated = 0;
    next_gc = 1024 * 1024;
    root_count = 0;
}

void* gc_allocate(size_t size, ObjType type) {
    bytes_allocated += size;
    Obj* obj = (Obj*)malloc(size);
    if (!obj) {
        fprintf(stderr, "MeteScript Bellek Hatası: Yetersiz bellek!\n");
        exit(1);
    }
    obj->type = type;
    obj->marked = false;
    obj->next = all_objects;
    all_objects = obj;
    return obj;
}

void gc_push_root(Value val) {
    if (root_count < ROOT_STACK_MAX) {
        root_stack[root_count++] = val;
    }
}

void gc_pop_roots(size_t count) {
    if (root_count >= count) {
        root_count -= count;
    } else {
        root_count = 0;
    }
}

void gc_mark_value(Value val) {
    if (val.type == VAL_OBJ && val.as.obj != NULL) {
        gc_mark_obj(val.as.obj);
    }
}

void gc_mark_obj(Obj* obj) {
    if (obj == NULL || obj->marked) return;

    obj->marked = true;

    switch (obj->type) {
        case OBJ_STRING:
            break;
        case OBJ_LIST: {
            ObjList* list = (ObjList*)obj;
            for (size_t i = 0; i < list->count; i++) {
                gc_mark_value(list->items[i]);
            }
            break;
        }
        case OBJ_MAP: {
            ObjMap* map = (ObjMap*)obj;
            for (size_t i = 0; i < map->count; i++) {
                gc_mark_value(map->entries[i].value);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* fn = (ObjFunction*)obj;
            if (fn->closure) {
                gc_mark_obj((Obj*)fn->closure);
            }
            break;
        }
        case OBJ_NATIVE:
            break;
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bm = (ObjBoundMethod*)obj;
            gc_mark_value(bm->receiver);
            gc_mark_value(bm->method);
            break;
        }
        case OBJ_WINDOW:
            break;
        case OBJ_ENV: {
            Environment* env = (Environment*)obj;
            for (size_t i = 0; i < env->count; i++) {
                gc_mark_value(env->entries[i].value);
            }
            if (env->enclosing) {
                gc_mark_obj((Obj*)env->enclosing);
            }
            break;
        }
    }
}

void gc_mark_env(Environment* env) {
    if (env != NULL) {
        gc_mark_obj((Obj*)env);
    }
}

void gui_window_destroy(void* win);

static void free_object(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString* str = (ObjString*)obj;
            free(str->chars);
            free(str);
            break;
        }
        case OBJ_LIST: {
            ObjList* list = (ObjList*)obj;
            free(list->items);
            free(list);
            break;
        }
        case OBJ_MAP: {
            ObjMap* map = (ObjMap*)obj;
            for (size_t i = 0; i < map->count; i++) {
                free(map->entries[i].key);
            }
            free(map->entries);
            free(map);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* fn = (ObjFunction*)obj;
            free(fn->name);
            for (size_t i = 0; i < fn->arity; i++) {
                free(fn->params[i]);
            }
            free(fn->params);
            free(fn);
            break;
        }
        case OBJ_NATIVE: {
            ObjNative* nat = (ObjNative*)obj;
            free(nat->name);
            free(nat);
            break;
        }
        case OBJ_BOUND_METHOD: {
            free(obj);
            break;
        }
        case OBJ_WINDOW: {
            gui_window_destroy(obj);
            free(obj);
            break;
        }
        case OBJ_ENV: {
            Environment* env = (Environment*)obj;
            for (size_t i = 0; i < env->count; i++) {
                free(env->entries[i].key);
            }
            free(env->entries);
            free(env);
            break;
        }
    }
}

void gc_collect(Environment* active_env) {
    // 1. Mark roots
    for (size_t i = 0; i < root_count; i++) {
        gc_mark_value(root_stack[i]);
    }
    if (active_env) {
        gc_mark_env(active_env);
    }

    // 2. Sweep
    Obj** prev = &all_objects;
    while (*prev != NULL) {
        Obj* current = *prev;
        if (!current->marked) {
            *prev = current->next;
            free_object(current);
        } else {
            current->marked = false; // reset for next cycle
            prev = &current->next;
        }
    }

    next_gc = bytes_allocated * 2;
    if (next_gc < 1024 * 1024) next_gc = 1024 * 1024;
}

void gc_free_all(void) {
    Obj* current = all_objects;
    while (current != NULL) {
        Obj* next = current->next;
        free_object(current);
        current = next;
    }
    all_objects = NULL;
    bytes_allocated = 0;
    root_count = 0;
}
