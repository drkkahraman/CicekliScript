#ifndef CICEKLISCRIPT_TENSOR_H
#define CICEKLISCRIPT_TENSOR_H

#include "value.h"
#include <stdbool.h>

typedef enum {
    DEVICE_CPU = 0,
    DEVICE_CUDA = 1
} TensorDevice;

typedef struct ObjTensor {
    Obj obj;
    float* data;       // CPU host memory
    float* cuda_data;  // CUDA GPU memory
    int rows;
    int cols;
    int total_elements;
    TensorDevice device;
} ObjTensor;

ObjTensor* new_tensor(int rows, int cols, TensorDevice device);
void tensor_free_data(ObjTensor* t);

Value native_tensor_create(Value self, size_t argc, Value* args, int line);
Value native_tensor_randn(Value self, size_t argc, Value* args, int line);
Value native_tensor_zeros(Value self, size_t argc, Value* args, int line);
Value native_tensor_ones(Value self, size_t argc, Value* args, int line);

Value native_cuda_is_available(Value self, size_t argc, Value* args, int line);
Value native_cuda_device_name(Value self, size_t argc, Value* args, int line);
Value native_cuda_device_count(Value self, size_t argc, Value* args, int line);
Value native_cuda_memory_info(Value self, size_t argc, Value* args, int line);

Value tensor_get_method(Value tensor_val, const char* name);
Value tensor_get_property(Value tensor_val, const char* name);

char* tensor_to_string(ObjTensor* t);

#endif // CICEKLISCRIPT_TENSOR_H
