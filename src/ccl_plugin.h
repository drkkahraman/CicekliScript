#ifndef CCL_PLUGIN_H
#define CCL_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CCL_NIL = 0,
    CCL_BOOL = 1,
    CCL_NUM = 2,
    CCL_STR = 3
} CCLValType;

typedef struct {
    CCLValType type;
    union {
        int b;
        double n;
        const char* s;
    } as;
} CCLVal;

static inline CCLVal ccl_nil(void) {
    CCLVal v; v.type = CCL_NIL; v.as.n = 0; return v;
}

static inline CCLVal ccl_bool(int b) {
    CCLVal v; v.type = CCL_BOOL; v.as.b = b ? 1 : 0; return v;
}

static inline CCLVal ccl_num(double n) {
    CCLVal v; v.type = CCL_NUM; v.as.n = n; return v;
}

static inline CCLVal ccl_str(const char* s) {
    CCLVal v; v.type = CCL_STR; v.as.s = s ? s : ""; return v;
}

static inline double ccl_as_num(CCLVal v) {
    return (v.type == CCL_NUM) ? v.as.n : (v.type == CCL_BOOL ? (double)v.as.b : 0.0);
}

static inline int ccl_as_bool(CCLVal v) {
    if (v.type == CCL_BOOL) return v.as.b;
    if (v.type == CCL_NUM) return v.as.n != 0.0;
    if (v.type == CCL_STR) return v.as.s && v.as.s[0] != '\0';
    return 0;
}

static inline const char* ccl_as_str(CCLVal v) {
    return (v.type == CCL_STR && v.as.s) ? v.as.s : "";
}

typedef CCLVal (*CCLPluginFunc)(int argc, CCLVal* argv);
typedef void (*CCLRegisterFn)(const char* name, CCLPluginFunc fn, int arity);

/* Every C library must export: void ccl_init(CCLRegisterFn reg); */

#ifdef __cplusplus
}
#endif

#endif /* CCL_PLUGIN_H */
