#include "gpu_cuda.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TILE_DIM 16
#define BLOCK_SIZE 256

__global__ void k_matmul_tiled(const float* __restrict__ A, const float* __restrict__ B, float* __restrict__ C, int M, int K, int N) {
    __shared__ float sA[TILE_DIM][TILE_DIM];
    __shared__ float sB[TILE_DIM][TILE_DIM];

    int row = blockIdx.y * TILE_DIM + threadIdx.y;
    int col = blockIdx.x * TILE_DIM + threadIdx.x;
    float sum = 0.0f;

    int num_tiles = (K + TILE_DIM - 1) / TILE_DIM;
    for (int t = 0; t < num_tiles; t++) {
        int a_col = t * TILE_DIM + threadIdx.x;
        if (row < M && a_col < K) {
            sA[threadIdx.y][threadIdx.x] = A[row * K + a_col];
        } else {
            sA[threadIdx.y][threadIdx.x] = 0.0f;
        }

        int b_row = t * TILE_DIM + threadIdx.y;
        if (b_row < K && col < N) {
            sB[threadIdx.y][threadIdx.x] = B[b_row * N + col];
        } else {
            sB[threadIdx.y][threadIdx.x] = 0.0f;
        }

        __syncthreads();

        #pragma unroll
        for (int k = 0; k < TILE_DIM; k++) {
            sum += sA[threadIdx.y][k] * sB[k][threadIdx.x];
        }
        __syncthreads();
    }

    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

__global__ void k_add(const float* A, const float* B, float* C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) C[idx] = A[idx] + B[idx];
}

__global__ void k_sub(const float* A, const float* B, float* C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) C[idx] = A[idx] - B[idx];
}

__global__ void k_mul(const float* A, const float* B, float* C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) C[idx] = A[idx] * B[idx];
}

__global__ void k_scale(const float* A, float scalar, float* C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) C[idx] = A[idx] * scalar;
}

__global__ void k_fill(float* A, float val, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) A[idx] = val;
}

__global__ void k_relu(const float* A, float* C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        float v = A[idx];
        C[idx] = (v > 0.0f) ? v : 0.0f;
    }
}

__global__ void k_sigmoid(const float* A, float* C, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        C[idx] = 1.0f / (1.0f + expf(-A[idx]));
    }
}

__global__ void k_sum_reduction(const float* A, float* partial_sums, int n) {
    __shared__ float sdata[BLOCK_SIZE];
    unsigned int tid = threadIdx.x;
    unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;

    sdata[tid] = (i < n) ? A[i] : 0.0f;
    __syncthreads();

    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) partial_sums[blockIdx.x] = sdata[0];
}

extern "C" {

bool gpu_cuda_is_available(void) {
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    return (err == cudaSuccess && count > 0);
}

const char* gpu_cuda_device_name(void) {
    static char dev_name[256] = "CPU (No CUDA GPU)";
    int count = 0;
    if (cudaGetDeviceCount(&count) == cudaSuccess && count > 0) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            snprintf(dev_name, sizeof(dev_name), "%s", prop.name);
            return dev_name;
        }
    }
    return dev_name;
}

int gpu_cuda_device_count(void) {
    int count = 0;
    if (cudaGetDeviceCount(&count) == cudaSuccess) {
        return count;
    }
    return 0;
}

void gpu_cuda_memory_info(size_t* free_bytes, size_t* total_bytes) {
    size_t f = 0, t = 0;
    if (cudaMemGetInfo(&f, &t) == cudaSuccess) {
        if (free_bytes) *free_bytes = f;
        if (total_bytes) *total_bytes = t;
    } else {
        if (free_bytes) *free_bytes = 0;
        if (total_bytes) *total_bytes = 0;
    }
}

float* gpu_cuda_alloc(size_t elements) {
    float* d_ptr = NULL;
    cudaError_t err = cudaMalloc((void**)&d_ptr, elements * sizeof(float));
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA out of memory error: %s\n", cudaGetErrorString(err));
        return NULL;
    }
    return d_ptr;
}

void gpu_cuda_free(float* ptr) {
    if (ptr) {
        cudaFree(ptr);
    }
}

void gpu_cuda_memcpy_h2d(float* d_dst, const float* h_src, size_t elements) {
    if (d_dst && h_src && elements > 0) {
        cudaMemcpy(d_dst, h_src, elements * sizeof(float), cudaMemcpyHostToDevice);
    }
}

void gpu_cuda_memcpy_d2h(float* h_dst, const float* d_src, size_t elements) {
    if (h_dst && d_src && elements > 0) {
        cudaMemcpy(h_dst, d_src, elements * sizeof(float), cudaMemcpyDeviceToHost);
    }
}

void gpu_cuda_fill(float* d_dst, float value, int n) {
    if (!d_dst || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_fill<<<grid, BLOCK_SIZE>>>(d_dst, value, n);
}

void gpu_cuda_matmul(const float* A, const float* B, float* C, int M, int K, int N) {
    if (!A || !B || !C || M <= 0 || K <= 0 || N <= 0) return;
    dim3 block(TILE_DIM, TILE_DIM);
    dim3 grid((N + TILE_DIM - 1) / TILE_DIM, (M + TILE_DIM - 1) / TILE_DIM);
    k_matmul_tiled<<<grid, block>>>(A, B, C, M, K, N);
}

void gpu_cuda_add(const float* A, const float* B, float* C, int n) {
    if (!A || !B || !C || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_add<<<grid, BLOCK_SIZE>>>(A, B, C, n);
}

void gpu_cuda_sub(const float* A, const float* B, float* C, int n) {
    if (!A || !B || !C || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_sub<<<grid, BLOCK_SIZE>>>(A, B, C, n);
}

void gpu_cuda_mul(const float* A, const float* B, float* C, int n) {
    if (!A || !B || !C || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_mul<<<grid, BLOCK_SIZE>>>(A, B, C, n);
}

void gpu_cuda_scale(const float* A, float scalar, float* C, int n) {
    if (!A || !C || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_scale<<<grid, BLOCK_SIZE>>>(A, scalar, C, n);
}

void gpu_cuda_relu(const float* A, float* C, int n) {
    if (!A || !C || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_relu<<<grid, BLOCK_SIZE>>>(A, C, n);
}

void gpu_cuda_sigmoid(const float* A, float* C, int n) {
    if (!A || !C || n <= 0) return;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    k_sigmoid<<<grid, BLOCK_SIZE>>>(A, C, n);
}

float gpu_cuda_sum(const float* A, int n) {
    if (!A || n <= 0) return 0.0f;
    int grid = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    float* d_partials = NULL;
    cudaMalloc((void**)&d_partials, grid * sizeof(float));

    k_sum_reduction<<<grid, BLOCK_SIZE>>>(A, d_partials, n);

    float* h_partials = (float*)malloc(grid * sizeof(float));
    cudaMemcpy(h_partials, d_partials, grid * sizeof(float), cudaMemcpyDeviceToHost);
    cudaFree(d_partials);

    float total = 0.0f;
    for (int i = 0; i < grid; i++) {
        total += h_partials[i];
    }
    free(h_partials);
    return total;
}

void gpu_cuda_synchronize(void) {
    cudaDeviceSynchronize();
}

} // extern "C"
