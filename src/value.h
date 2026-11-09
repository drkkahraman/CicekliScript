#ifndef CICEKLISCRIPT_VALUE_H
#define CICEKLISCRIPT_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef enum {
    OBJ_STRING,
    OBJ_LIST,
    OBJ_MAP,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_ENV,
    OBJ_BOUND_METHOD,
    OBJ_WINDOW,
    OBJ_TENSOR
} ObjType;

struct Obj;
struct Environment;
struct Stmt;

typedef struct Obj {
    ObjType type;
    bool marked;
    struct Obj* next;
} Obj;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

// String object
typedef struct {
    Obj obj;
    char* chars;
    size_t length;
    uint32_t hash;
} ObjString;

// Dynamic list object
typedef struct {
    Obj obj;
    Value* items;
    size_t count;
    size_t capacity;
} ObjList;

// Map entry
typedef struct {
    char* key;
    Value value;
} MapEntry;

// Map / Object
typedef struct {
    Obj obj;
    MapEntry* entries;
    size_t count;
    size_t capacity;
} ObjMap;

// Function object
typedef struct {
    Obj obj;
    char* name;
    char** params;
    size_t arity;
    struct Stmt* body;
    struct Environment* closure;
} ObjFunction;

// Native function pointer type
typedef Value (*NativeFn)(Value self, size_t arg_count, Value* args, int line);

typedef struct {
    Obj obj;
    char* name;
    NativeFn fn;
    int arity; // -1 for variable arguments
} ObjNative;

// Bound Method object (receiver + method)
typedef struct {
    Obj obj;
    Value receiver;
    Value method;
} ObjBoundMethod;

// Value Constructors
Value make_null(void);
Value make_bool(bool b);
Value make_number(double n);
Value make_obj(Obj* o);

// Helpers
static inline bool is_null(Value v) { return v.type == VAL_NULL; }
static inline bool is_bool(Value v) { return v.type == VAL_BOOL; }
static inline bool is_number(Value v) { return v.type == VAL_NUMBER; }
static inline bool is_obj(Value v) { return v.type == VAL_OBJ; }
static inline bool is_obj_type(Value v, ObjType type) {
    return is_obj(v) && v.as.obj != NULL && v.as.obj->type == type;
}
static inline bool is_string(Value v) { return is_obj_type(v, OBJ_STRING); }
static inline bool is_list(Value v) { return is_obj_type(v, OBJ_LIST); }
static inline bool is_map(Value v) { return is_obj_type(v, OBJ_MAP); }
static inline bool is_function(Value v) { return is_obj_type(v, OBJ_FUNCTION); }
static inline bool is_native(Value v) { return is_obj_type(v, OBJ_NATIVE); }
static inline bool is_bound_method(Value v) { return is_obj_type(v, OBJ_BOUND_METHOD); }
static inline bool is_window(Value v) { return is_obj_type(v, OBJ_WINDOW); }
static inline bool is_tensor(Value v) { return is_obj_type(v, OBJ_TENSOR); }

static inline bool as_bool(Value v) { return v.as.boolean; }
static inline double as_number(Value v) { return v.as.number; }
static inline Obj* as_obj(Value v) { return v.as.obj; }
static inline ObjString* as_string(Value v) { return (ObjString*)v.as.obj; }
static inline ObjList* as_list(Value v) { return (ObjList*)v.as.obj; }
static inline ObjMap* as_map(Value v) { return (ObjMap*)v.as.obj; }
static inline ObjFunction* as_function(Value v) { return (ObjFunction*)v.as.obj; }
static inline ObjNative* as_native(Value v) { return (ObjNative*)v.as.obj; }
static inline ObjBoundMethod* as_bound_method(Value v) { return (ObjBoundMethod*)v.as.obj; }

bool values_equal(Value a, Value b);
bool is_truthy(Value v);
void print_value(Value v);
char* value_to_string(Value v);
const char* value_type_name(Value v);

// Object allocation & list/map management
ObjString* new_string(const char* chars, size_t length);
ObjString* copy_string(const char* chars);
ObjString* take_string(char* chars, size_t length);

ObjList* new_list(void);
void list_append(ObjList* list, Value v);
bool list_set(ObjList* list, int64_t index, Value v);
bool list_get(ObjList* list, int64_t index, Value* out);
Value list_pop(ObjList* list, int64_t index);

ObjMap* new_map(void);
void map_set(ObjMap* map, const char* key, Value value);
bool map_get(ObjMap* map, const char* key, Value* out);

ObjFunction* new_function(const char* name, char** params, size_t arity, struct Stmt* body, struct Environment* closure);
ObjNative* new_native(const char* name, NativeFn fn, int arity);
ObjBoundMethod* new_bound_method(Value receiver, Value method);

#endif // CICEKLISCRIPT_VALUE_H
