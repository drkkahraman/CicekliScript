#define _POSIX_C_SOURCE 200809L
#include "tensor.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef USE_CUDA
#include "gpu_cuda.h"
#else
static inline bool gpu_cuda_is_available(void) { return false; }
static inline const char* gpu_cuda_device_name(void) { return "CPU (CUDA compiled out)"; }
static inline int gpu_cuda_device_count(void) { return 0; }
static inline void gpu_cuda_memory_info(size_t* f, size_t* t) { if(f)*f=0; if(t)*t=0; }
static inline float* gpu_cuda_alloc(size_t e) { (void)e; return NULL; }
static inline void gpu_cuda_free(float* p) { (void)p; }
static inline void gpu_cuda_memcpy_h2d(float* d, const float* s, size_t e) { (void)d; (void)s; (void)e; }
static inline void gpu_cuda_memcpy_d2h(float* d, const float* s, size_t e) { (void)d; (void)s; (void)e; }
static inline void gpu_cuda_fill(float* d, float v, int n) { (void)d; (void)v; (void)n; }
static inline void gpu_cuda_matmul(const float* A, const float* B, float* C, int M, int K, int N) { (void)A;(void)B;(void)C;(void)M;(void)K;(void)N; }
static inline void gpu_cuda_add(const float* A, const float* B, float* C, int n) { (void)A;(void)B;(void)C;(void)n; }
static inline void gpu_cuda_sub(const float* A, const float* B, float* C, int n) { (void)A;(void)B;(void)C;(void)n; }
static inline void gpu_cuda_mul(const float* A, const float* B, float* C, int n) { (void)A;(void)B;(void)C;(void)n; }
static inline void gpu_cuda_scale(const float* A, float s, float* C, int n) { (void)A;(void)s;(void)C;(void)n; }
static inline void gpu_cuda_relu(const float* A, float* C, int n) { (void)A;(void)C;(void)n; }
static inline void gpu_cuda_sigmoid(const float* A, float* C, int n) { (void)A;(void)C;(void)n; }
static inline float gpu_cuda_sum(const float* A, int n) { (void)A;(void)n; return 0.0f; }
static inline void gpu_cuda_synchronize(void) {}
#endif

ObjTensor* new_tensor(int rows, int cols, TensorDevice device) {
    if (rows <= 0) rows = 1;
    if (cols <= 0) cols = 1;

    ObjTensor* t = (ObjTensor*)gc_allocate(sizeof(ObjTensor), OBJ_TENSOR);
    t->rows = rows;
    t->cols = cols;
    t->total_elements = rows * cols;
    t->device = device;
    t->data = (float*)calloc(t->total_elements, sizeof(float));
    t->cuda_data = NULL;

    if (device == DEVICE_CUDA) {
        if (gpu_cuda_is_available()) {
            t->cuda_data = gpu_cuda_alloc(t->total_elements);
        } else {
            // Fallback to CPU if CUDA requested but not available
            t->device = DEVICE_CPU;
        }
    }

    return t;
}

void tensor_free_data(ObjTensor* t) {
    if (!t) return;
    if (t->data) {
        free(t->data);
        t->data = NULL;
    }
    if (t->cuda_data) {
        gpu_cuda_free(t->cuda_data);
        t->cuda_data = NULL;
    }
}

// Ensure host data is up to date (sync from CUDA if needed)
static void tensor_sync_to_host(ObjTensor* t) {
    if (!t) return;
    if (t->device == DEVICE_CUDA && t->cuda_data) {
        if (!t->data) {
            t->data = (float*)malloc(t->total_elements * sizeof(float));
        }
        gpu_cuda_memcpy_d2h(t->data, t->cuda_data, t->total_elements);
    }
}

// Convert tensor to string for print / REPL
char* tensor_to_string(ObjTensor* t) {
    if (!t) return strdup("tensor(null)");
    tensor_sync_to_host(t);

    char buffer[4096];
    size_t offset = 0;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "tensor([");

    for (int r = 0; r < t->rows && offset + 50 < sizeof(buffer); r++) {
        if (t->rows > 1) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n  [");
        }
        for (int c = 0; c < t->cols && offset + 30 < sizeof(buffer); c++) {
            float val = t->data ? t->data[r * t->cols + c] : 0.0f;
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                               c == 0 ? "%.4f" : ", %.4f", val);
        }
        if (t->rows > 1) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "]");
        }
    }

    if (t->rows > 1) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n], shape=[%d, %d], device='%s')",
                           t->rows, t->cols, t->device == DEVICE_CUDA ? "cuda:0" : "cpu");
    } else {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "], shape=[%d, %d], device='%s')",
                           t->rows, t->cols, t->device == DEVICE_CUDA ? "cuda:0" : "cpu");
    }

    return strdup(buffer);
}

// ============================================================================
// Tensor Factory Built-ins
// ============================================================================

// cicekli.tensor(liste_veya_sayilar, [shape=[rows, cols]], [device="cpu"/"cuda"])
Value native_tensor_create(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)line;
    if (argc < 1) {
        return make_obj((Obj*)new_tensor(1, 1, DEVICE_CPU));
    }

    TensorDevice dev = DEVICE_CPU;
    int rows = 1;
    int cols = 1;

    // Check device from last argument if string
    if (argc >= 2 && is_string(args[argc - 1])) {
        const char* dev_str = as_string(args[argc - 1])->chars;
        if (strcmp(dev_str, "cuda") == 0 || strcmp(dev_str, "gpu") == 0) {
            dev = DEVICE_CUDA;
        }
    }

    // Check shape from args[1] if list
    if (argc >= 2 && is_list(args[1])) {
        ObjList* sh = as_list(args[1]);
        if (sh->count >= 1 && is_number(sh->items[0])) rows = (int)as_number(sh->items[0]);
        if (sh->count >= 2 && is_number(sh->items[1])) cols = (int)as_number(sh->items[1]);
    }

    // If 1st arg is a nested list: e.g. [[1, 2], [3, 4]]
    if (is_list(args[0])) {
        ObjList* outer = as_list(args[0]);
        if (outer->count > 0 && is_list(outer->items[0])) {
            rows = (int)outer->count;
            cols = (int)as_list(outer->items[0])->count;
            ObjTensor* t = new_tensor(rows, cols, dev);
            float* host = (float*)malloc(rows * cols * sizeof(float));

            for (int r = 0; r < rows; r++) {
                if (is_list(outer->items[r])) {
                    ObjList* inner = as_list(outer->items[r]);
                    for (int c = 0; c < cols; c++) {
                        float v = 0.0f;
                        if (c < (int)inner->count && is_number(inner->items[c])) {
                            v = (float)as_number(inner->items[c]);
                        }
                        host[r * cols + c] = v;
                    }
                }
            }
            if (dev == DEVICE_CUDA && t->cuda_data) {
                gpu_cuda_memcpy_h2d(t->cuda_data, host, rows * cols);
            } else {
                memcpy(t->data, host, rows * cols * sizeof(float));
            }
            free(host);
            return make_obj((Obj*)t);
        } else {
            // Flat list: [1, 2, 3, 4]
            int count = (int)outer->count;
            if (rows * cols != count) {
                rows = 1;
                cols = count;
            }
            ObjTensor* t = new_tensor(rows, cols, dev);
            float* host = (float*)malloc(count * sizeof(float));
            for (int i = 0; i < count; i++) {
                host[i] = is_number(outer->items[i]) ? (float)as_number(outer->items[i]) : 0.0f;
            }
            if (dev == DEVICE_CUDA && t->cuda_data) {
                gpu_cuda_memcpy_h2d(t->cuda_data, host, count);
            } else {
                memcpy(t->data, host, count * sizeof(float));
            }
            free(host);
            return make_obj((Obj*)t);
        }
    }

    // Default 1x1 tensor from number
    if (is_number(args[0])) {
        ObjTensor* t = new_tensor(1, 1, dev);
        float val = (float)as_number(args[0]);
        if (dev == DEVICE_CUDA && t->cuda_data) {
            gpu_cuda_memcpy_h2d(t->cuda_data, &val, 1);
        } else {
            t->data[0] = val;
        }
        return make_obj((Obj*)t);
    }

    return make_obj((Obj*)new_tensor(rows, cols, dev));
}

// cicekli.randn(rows, cols, [device="cpu"/"cuda"])
Value native_tensor_randn(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)line;
    int rows = 1, cols = 1;
    TensorDevice dev = DEVICE_CPU;

    if (argc >= 1 && is_number(args[0])) rows = (int)as_number(args[0]);
    if (argc >= 2 && is_number(args[1])) cols = (int)as_number(args[1]);
    if (argc >= 3 && is_string(args[2])) {
        if (strcmp(as_string(args[2])->chars, "cuda") == 0 || strcmp(as_string(args[2])->chars, "gpu") == 0) {
            dev = DEVICE_CUDA;
        }
    }

    ObjTensor* t = new_tensor(rows, cols, dev);
    int total = rows * cols;
    float* host = (float*)malloc(total * sizeof(float));

    // Box-Muller standard normal distribution
    for (int i = 0; i < total; i++) {
        float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 2.0f);
        float u2 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 2.0f);
        host[i] = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * 3.14159265f * u2);
    }

    if (dev == DEVICE_CUDA && t->cuda_data) {
        gpu_cuda_memcpy_h2d(t->cuda_data, host, total);
    } else {
        memcpy(t->data, host, total * sizeof(float));
    }
    free(host);

    return make_obj((Obj*)t);
}

// cicekli.zeros(rows, cols, [device])
Value native_tensor_zeros(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)line;
    int rows = 1, cols = 1;
    TensorDevice dev = DEVICE_CPU;
    if (argc >= 1 && is_number(args[0])) rows = (int)as_number(args[0]);
    if (argc >= 2 && is_number(args[1])) cols = (int)as_number(args[1]);
    if (argc >= 3 && is_string(args[2])) {
        if (strcmp(as_string(args[2])->chars, "cuda") == 0) dev = DEVICE_CUDA;
    }
    return make_obj((Obj*)new_tensor(rows, cols, dev));
}

// cicekli.ones(rows, cols, [device])
Value native_tensor_ones(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)line;
    int rows = 1, cols = 1;
    TensorDevice dev = DEVICE_CPU;
    if (argc >= 1 && is_number(args[0])) rows = (int)as_number(args[0]);
    if (argc >= 2 && is_number(args[1])) cols = (int)as_number(args[1]);
    if (argc >= 3 && is_string(args[2])) {
        if (strcmp(as_string(args[2])->chars, "cuda") == 0) dev = DEVICE_CUDA;
    }
    ObjTensor* t = new_tensor(rows, cols, dev);
    int total = rows * cols;
    if (dev == DEVICE_CUDA && t->cuda_data) {
        gpu_cuda_fill(t->cuda_data, 1.0f, total);
    } else {
        for (int i = 0; i < total; i++) t->data[i] = 1.0f;
    }
    return make_obj((Obj*)t);
}

// ============================================================================
// CUDA Subsystem Query Built-ins
// ============================================================================

Value native_cuda_is_available(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)argc; (void)args; (void)line;
    return make_bool(gpu_cuda_is_available());
}

Value native_cuda_device_name(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)argc; (void)args; (void)line;
    return make_obj((Obj*)copy_string(gpu_cuda_device_name()));
}

Value native_cuda_device_count(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)argc; (void)args; (void)line;
    return make_number(gpu_cuda_device_count());
}

Value native_cuda_memory_info(Value self, size_t argc, Value* args, int line) {
    (void)self; (void)argc; (void)args; (void)line;
    size_t free_b = 0, total_b = 0;
    gpu_cuda_memory_info(&free_b, &total_b);

    ObjMap* map = new_map();
    map_set(map, "free_mb", make_number((double)(free_b / (1024 * 1024))));
    map_set(map, "total_mb", make_number((double)(total_b / (1024 * 1024))));
    map_set(map, "free_bytes", make_number((double)free_b));
    map_set(map, "total_bytes", make_number((double)total_b));
    return make_obj((Obj*)map);
}

// ============================================================================
// Tensor Instance Methods
// ============================================================================

// tensor.to("cuda" / "cpu")
static Value method_tensor_to(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_tensor(self) || argc < 1 || !is_string(args[0])) return self;
    ObjTensor* src = (ObjTensor*)self.as.obj;
    const char* target = as_string(args[0])->chars;

    if (strcmp(target, "cuda") == 0 || strcmp(target, "gpu") == 0) {
        if (src->device == DEVICE_CUDA) return self;
        if (!gpu_cuda_is_available()) return self;

        ObjTensor* dst = new_tensor(src->rows, src->cols, DEVICE_CUDA);
        if (src->data && dst->cuda_data) {
            gpu_cuda_memcpy_h2d(dst->cuda_data, src->data, src->total_elements);
        }
        return make_obj((Obj*)dst);
    } else if (strcmp(target, "cpu") == 0) {
        if (src->device == DEVICE_CPU) return self;
        ObjTensor* dst = new_tensor(src->rows, src->cols, DEVICE_CPU);
        if (src->cuda_data && dst->data) {
            gpu_cuda_memcpy_d2h(dst->data, src->cuda_data, src->total_elements);
        }
        return make_obj((Obj*)dst);
    }
    return self;
}

static Value method_tensor_cuda(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    Value dev = make_obj((Obj*)copy_string("cuda"));
    return method_tensor_to(self, 1, &dev, line);
}

static Value method_tensor_cpu(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    Value dev = make_obj((Obj*)copy_string("cpu"));
    return method_tensor_to(self, 1, &dev, line);
}

// tensor.matmul(other)
static Value method_tensor_matmul(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_tensor(self) || argc < 1 || !is_tensor(args[0])) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;
    ObjTensor* B = (ObjTensor*)args[0].as.obj;

    if (A->cols != B->rows) {
        fprintf(stderr, "Matmul boyut uyuşmazlığı: [%d, %d] x [%d, %d] çarpılamaz!\n",
                A->rows, A->cols, B->rows, B->cols);
        return make_null();
    }

    int M = A->rows;
    int K = A->cols;
    int N = B->cols;

    TensorDevice dev = (A->device == DEVICE_CUDA || B->device == DEVICE_CUDA) ? DEVICE_CUDA : DEVICE_CPU;
    if (dev == DEVICE_CUDA && !gpu_cuda_is_available()) dev = DEVICE_CPU;

    ObjTensor* C = new_tensor(M, N, dev);

    if (dev == DEVICE_CUDA) {
        // If one is on CPU, transfer to CUDA
        float* d_A = A->cuda_data;
        float* d_B = B->cuda_data;
        bool free_a = false, free_b = false;

        if (!d_A && A->data) {
            d_A = gpu_cuda_alloc(A->total_elements);
            gpu_cuda_memcpy_h2d(d_A, A->data, A->total_elements);
            free_a = true;
        }
        if (!d_B && B->data) {
            d_B = gpu_cuda_alloc(B->total_elements);
            gpu_cuda_memcpy_h2d(d_B, B->data, B->total_elements);
            free_b = true;
        }

        gpu_cuda_matmul(d_A, d_B, C->cuda_data, M, K, N);
        gpu_cuda_synchronize();

        if (free_a) gpu_cuda_free(d_A);
        if (free_b) gpu_cuda_free(d_B);
    } else {
        // CPU Matrix Multiplication
        tensor_sync_to_host(A);
        tensor_sync_to_host(B);
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                float sum = 0.0f;
                for (int k = 0; k < K; k++) {
                    sum += A->data[i * K + k] * B->data[k * N + j];
                }
                C->data[i * N + j] = sum;
            }
        }
    }

    return make_obj((Obj*)C);
}

// Element-wise arithmetic
static Value method_tensor_add(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_tensor(self) || argc < 1) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;

    if (is_number(args[0])) {
        float s = (float)as_number(args[0]);
        ObjTensor* C = new_tensor(A->rows, A->cols, A->device);
        tensor_sync_to_host(A);
        for (int i = 0; i < A->total_elements; i++) C->data[i] = A->data[i] + s;
        if (A->device == DEVICE_CUDA && gpu_cuda_is_available()) {
            gpu_cuda_memcpy_h2d(C->cuda_data, C->data, A->total_elements);
        }
        return make_obj((Obj*)C);
    }

    if (!is_tensor(args[0])) return make_null();
    ObjTensor* B = (ObjTensor*)args[0].as.obj;

    if (A->total_elements != B->total_elements) {
        fprintf(stderr, "Tensor boyut uyuşmazlığı: [%d, %d] + [%d, %d]\n", A->rows, A->cols, B->rows, B->cols);
        return make_null();
    }

    TensorDevice dev = (A->device == DEVICE_CUDA || B->device == DEVICE_CUDA) ? DEVICE_CUDA : DEVICE_CPU;
    ObjTensor* C = new_tensor(A->rows, A->cols, dev);

    if (dev == DEVICE_CUDA && gpu_cuda_is_available()) {
        float* d_A = A->cuda_data ? A->cuda_data : C->cuda_data;
        float* d_B = B->cuda_data ? B->cuda_data : C->cuda_data;
        gpu_cuda_add(d_A, d_B, C->cuda_data, A->total_elements);
    } else {
        tensor_sync_to_host(A);
        tensor_sync_to_host(B);
        for (int i = 0; i < A->total_elements; i++) C->data[i] = A->data[i] + B->data[i];
    }
    return make_obj((Obj*)C);
}

static Value method_tensor_sub(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_tensor(self) || argc < 1) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;

    if (is_number(args[0])) {
        float s = (float)as_number(args[0]);
        ObjTensor* C = new_tensor(A->rows, A->cols, A->device);
        tensor_sync_to_host(A);
        for (int i = 0; i < A->total_elements; i++) C->data[i] = A->data[i] - s;
        if (A->device == DEVICE_CUDA && gpu_cuda_is_available()) {
            gpu_cuda_memcpy_h2d(C->cuda_data, C->data, A->total_elements);
        }
        return make_obj((Obj*)C);
    }

    if (!is_tensor(args[0])) return make_null();
    ObjTensor* B = (ObjTensor*)args[0].as.obj;
    if (A->total_elements != B->total_elements) return make_null();

    TensorDevice dev = (A->device == DEVICE_CUDA || B->device == DEVICE_CUDA) ? DEVICE_CUDA : DEVICE_CPU;
    ObjTensor* C = new_tensor(A->rows, A->cols, dev);

    if (dev == DEVICE_CUDA && gpu_cuda_is_available()) {
        gpu_cuda_sub(A->cuda_data, B->cuda_data, C->cuda_data, A->total_elements);
    } else {
        tensor_sync_to_host(A);
        tensor_sync_to_host(B);
        for (int i = 0; i < A->total_elements; i++) C->data[i] = A->data[i] - B->data[i];
    }
    return make_obj((Obj*)C);
}

static Value method_tensor_mul(Value self, size_t argc, Value* args, int line) {
    (void)line;
    if (!is_tensor(self) || argc < 1) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;

    // Scalar multiply
    if (is_number(args[0])) {
        float s = (float)as_number(args[0]);
        ObjTensor* C = new_tensor(A->rows, A->cols, A->device);
        if (A->device == DEVICE_CUDA && gpu_cuda_is_available()) {
            gpu_cuda_scale(A->cuda_data, s, C->cuda_data, A->total_elements);
        } else {
            tensor_sync_to_host(A);
            for (int i = 0; i < A->total_elements; i++) C->data[i] = A->data[i] * s;
        }
        return make_obj((Obj*)C);
    }

    if (is_tensor(args[0])) {
        ObjTensor* B = (ObjTensor*)args[0].as.obj;
        if (A->total_elements != B->total_elements) return make_null();
        TensorDevice dev = (A->device == DEVICE_CUDA || B->device == DEVICE_CUDA) ? DEVICE_CUDA : DEVICE_CPU;
        ObjTensor* C = new_tensor(A->rows, A->cols, dev);
        if (dev == DEVICE_CUDA && gpu_cuda_is_available()) {
            gpu_cuda_mul(A->cuda_data, B->cuda_data, C->cuda_data, A->total_elements);
        } else {
            tensor_sync_to_host(A);
            tensor_sync_to_host(B);
            for (int i = 0; i < A->total_elements; i++) C->data[i] = A->data[i] * B->data[i];
        }
        return make_obj((Obj*)C);
    }
    return make_null();
}

static Value method_tensor_relu(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_tensor(self)) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;
    ObjTensor* C = new_tensor(A->rows, A->cols, A->device);

    if (A->device == DEVICE_CUDA && gpu_cuda_is_available()) {
        gpu_cuda_relu(A->cuda_data, C->cuda_data, A->total_elements);
    } else {
        tensor_sync_to_host(A);
        for (int i = 0; i < A->total_elements; i++) {
            float v = A->data[i];
            C->data[i] = (v > 0.0f) ? v : 0.0f;
        }
    }
    return make_obj((Obj*)C);
}

static Value method_tensor_sigmoid(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_tensor(self)) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;
    ObjTensor* C = new_tensor(A->rows, A->cols, A->device);

    if (A->device == DEVICE_CUDA && gpu_cuda_is_available()) {
        gpu_cuda_sigmoid(A->cuda_data, C->cuda_data, A->total_elements);
    } else {
        tensor_sync_to_host(A);
        for (int i = 0; i < A->total_elements; i++) {
            C->data[i] = 1.0f / (1.0f + expf(-A->data[i]));
        }
    }
    return make_obj((Obj*)C);
}

static Value method_tensor_sum(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_tensor(self)) return make_number(0.0);
    ObjTensor* A = (ObjTensor*)self.as.obj;

    if (A->device == DEVICE_CUDA && gpu_cuda_is_available()) {
        return make_number((double)gpu_cuda_sum(A->cuda_data, A->total_elements));
    } else {
        tensor_sync_to_host(A);
        double total = 0.0;
        for (int i = 0; i < A->total_elements; i++) total += A->data[i];
        return make_number(total);
    }
}

static Value method_tensor_mean(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_tensor(self)) return make_number(0.0);
    ObjTensor* A = (ObjTensor*)self.as.obj;
    Value s = method_tensor_sum(self, 0, NULL, line);
    return make_number(as_number(s) / (double)A->total_elements);
}

static Value method_tensor_tolist(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_tensor(self)) return make_null();
    ObjTensor* A = (ObjTensor*)self.as.obj;
    tensor_sync_to_host(A);

    ObjList* outer = new_list();
    if (A->rows > 1) {
        for (int r = 0; r < A->rows; r++) {
            ObjList* inner = new_list();
            for (int c = 0; c < A->cols; c++) {
                list_append(inner, make_number((double)A->data[r * A->cols + c]));
            }
            list_append(outer, make_obj((Obj*)inner));
        }
    } else {
        for (int c = 0; c < A->cols; c++) {
            list_append(outer, make_number((double)A->data[c]));
        }
    }
    return make_obj((Obj*)outer);
}

static Value method_tensor_print(Value self, size_t argc, Value* args, int line) {
    (void)argc; (void)args; (void)line;
    if (!is_tensor(self)) return make_null();
    char* str = tensor_to_string((ObjTensor*)self.as.obj);
    puts(str);
    free(str);
    return make_null();
}

// Method dispatcher for tensor.method(...)
Value tensor_get_method(Value tensor_val, const char* name) {
    if (!is_tensor(tensor_val)) return make_null();

    #define BIND_M(method_name, fn, arity) \
        if (strcmp(name, method_name) == 0) \
            return make_obj((Obj*)new_bound_method(tensor_val, make_obj((Obj*)new_native(method_name, fn, arity))))

    BIND_M("to", method_tensor_to, 1);
    BIND_M("cuda", method_tensor_cuda, 0);
    BIND_M("cpu", method_tensor_cpu, 0);
    BIND_M("matmul", method_tensor_matmul, 1);
    BIND_M("mm", method_tensor_matmul, 1);
    BIND_M("dot", method_tensor_matmul, 1);
    BIND_M("add", method_tensor_add, 1);
    BIND_M("sub", method_tensor_sub, 1);
    BIND_M("mul", method_tensor_mul, 1);
    BIND_M("relu", method_tensor_relu, 0);
    BIND_M("sigmoid", method_tensor_sigmoid, 0);
    BIND_M("sum", method_tensor_sum, 0);
    BIND_M("mean", method_tensor_mean, 0);
    BIND_M("tolist", method_tensor_tolist, 0);
    BIND_M("print", method_tensor_print, 0);

    #undef BIND_M
    return make_null();
}

// Property dispatcher for tensor.property (tensor.shape, tensor.device, tensor.rows, tensor.cols)
Value tensor_get_property(Value tensor_val, const char* name) {
    if (!is_tensor(tensor_val)) return make_null();
    ObjTensor* t = (ObjTensor*)tensor_val.as.obj;

    if (strcmp(name, "shape") == 0) {
        ObjList* sh = new_list();
        list_append(sh, make_number((double)t->rows));
        list_append(sh, make_number((double)t->cols));
        return make_obj((Obj*)sh);
    }
    if (strcmp(name, "device") == 0) {
        return make_obj((Obj*)copy_string(t->device == DEVICE_CUDA ? "cuda:0" : "cpu"));
    }
    if (strcmp(name, "rows") == 0) return make_number((double)t->rows);
    if (strcmp(name, "cols") == 0) return make_number((double)t->cols);
    if (strcmp(name, "elements") == 0) return make_number((double)t->total_elements);

    return make_null();
}
