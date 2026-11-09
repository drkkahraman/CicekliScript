#ifndef CICEKLISCRIPT_GPU_CUDA_H
#define CICEKLISCRIPT_GPU_CUDA_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool gpu_cuda_is_available(void);
const char* gpu_cuda_device_name(void);
int gpu_cuda_device_count(void);
void gpu_cuda_memory_info(size_t* free_bytes, size_t* total_bytes);

float* gpu_cuda_alloc(size_t elements);
void gpu_cuda_free(float* ptr);
void gpu_cuda_memcpy_h2d(float* d_dst, const float* h_src, size_t elements);
void gpu_cuda_memcpy_d2h(float* h_dst, const float* d_src, size_t elements);

void gpu_cuda_fill(float* d_dst, float value, int n);
void gpu_cuda_matmul(const float* A, const float* B, float* C, int M, int K, int N);
void gpu_cuda_add(const float* A, const float* B, float* C, int n);
void gpu_cuda_sub(const float* A, const float* B, float* C, int n);
void gpu_cuda_mul(const float* A, const float* B, float* C, int n);
void gpu_cuda_scale(const float* A, float scalar, float* C, int n);
void gpu_cuda_relu(const float* A, float* C, int n);
void gpu_cuda_sigmoid(const float* A, float* C, int n);
float gpu_cuda_sum(const float* A, int n);
void gpu_cuda_synchronize(void);

#ifdef __cplusplus
}
#endif

#endif // CICEKLISCRIPT_GPU_CUDA_H
