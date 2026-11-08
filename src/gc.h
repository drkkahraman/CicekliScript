#ifndef METESCRIPT_GC_H
#define METESCRIPT_GC_H

#include "value.h"
#include "env.h"

void gc_init(void);
void* gc_allocate(size_t size, ObjType type);
void gc_register_obj(Obj* obj);
void gc_push_root(Value val);
void gc_pop_roots(size_t count);
void gc_collect(Environment* active_env);
void gc_free_all(void);

void gc_mark_value(Value val);
void gc_mark_obj(Obj* obj);
void gc_mark_env(Environment* env);

#endif // METESCRIPT_GC_H
