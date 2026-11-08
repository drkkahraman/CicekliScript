#ifndef METESCRIPT_ENV_H
#define METESCRIPT_ENV_H

#include "value.h"

typedef struct {
    char* key;
    Value value;
} EnvEntry;

typedef struct Environment {
    Obj obj; // inherits Obj so GC can track it
    struct Environment* enclosing;
    EnvEntry* entries;
    size_t count;
    size_t capacity;
} Environment;

Environment* new_environment(Environment* enclosing);
void env_define(Environment* env, const char* name, Value value);
bool env_set(Environment* env, const char* name, Value value);
bool env_get(Environment* env, const char* name, Value* out);
bool env_set_at(Environment* env, int distance, const char* name, Value value);
bool env_get_at(Environment* env, int distance, const char* name, Value* out);

#endif // METESCRIPT_ENV_H
