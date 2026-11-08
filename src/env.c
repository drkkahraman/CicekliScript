#include "env.h"
#include "gc.h"
#include <stdlib.h>
#include <string.h>

Environment* new_environment(Environment* enclosing) {
    Environment* env = (Environment*)gc_allocate(sizeof(Environment), OBJ_ENV);
    env->enclosing = enclosing;
    env->entries = NULL;
    env->count = 0;
    env->capacity = 0;
    return env;
}

void env_define(Environment* env, const char* name, Value value) {
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->entries[i].key, name) == 0) {
            env->entries[i].value = value;
            return;
        }
    }
    if (env->count >= env->capacity) {
        size_t old_cap = env->capacity;
        env->capacity = old_cap < 8 ? 8 : old_cap * 2;
        env->entries = (EnvEntry*)realloc(env->entries, sizeof(EnvEntry) * env->capacity);
        if (!env->entries) exit(1);
    }
    env->entries[env->count].key = strdup(name);
    env->entries[env->count].value = value;
    env->count++;
}

bool env_set(Environment* env, const char* name, Value value) {
    Environment* curr = env;
    while (curr != NULL) {
        for (size_t i = 0; i < curr->count; i++) {
            if (strcmp(curr->entries[i].key, name) == 0) {
                curr->entries[i].value = value;
                return true;
            }
        }
        curr = curr->enclosing;
    }
    // If not found in any enclosing scope, define it in the current scope!
    env_define(env, name, value);
    return true;
}

bool env_get(Environment* env, const char* name, Value* out) {
    Environment* curr = env;
    while (curr != NULL) {
        for (size_t i = 0; i < curr->count; i++) {
            if (strcmp(curr->entries[i].key, name) == 0) {
                *out = curr->entries[i].value;
                return true;
            }
        }
        curr = curr->enclosing;
    }
    return false;
}

bool env_set_at(Environment* env, int distance, const char* name, Value value) {
    Environment* curr = env;
    for (int i = 0; i < distance && curr != NULL; i++) {
        curr = curr->enclosing;
    }
    if (!curr) return false;
    for (size_t i = 0; i < curr->count; i++) {
        if (strcmp(curr->entries[i].key, name) == 0) {
            curr->entries[i].value = value;
            return true;
        }
    }
    env_define(curr, name, value);
    return true;
}

bool env_get_at(Environment* env, int distance, const char* name, Value* out) {
    Environment* curr = env;
    for (int i = 0; i < distance && curr != NULL; i++) {
        curr = curr->enclosing;
    }
    if (!curr) return false;
    for (size_t i = 0; i < curr->count; i++) {
        if (strcmp(curr->entries[i].key, name) == 0) {
            *out = curr->entries[i].value;
            return true;
        }
    }
    return false;
}
