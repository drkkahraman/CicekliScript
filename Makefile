CC ?= gcc
NVCC ?= $(shell which nvcc 2>/dev/null || which /opt/cuda/bin/nvcc 2>/dev/null)
CUDA_PATH ?= /opt/cuda

PKG_CFLAGS = $(shell pkg-config --cflags cairo x11)
PKG_LIBS = $(shell pkg-config --libs cairo x11)

# CUDA auto-detection
ifneq ($(NVCC),)
    CUDA_CFLAGS = -DUSE_CUDA -I$(CUDA_PATH)/include
    CUDA_LDFLAGS = -L$(CUDA_PATH)/lib64 -lcudart -lstdc++
    CUDA_OBJS = src/gpu_cuda.o
    CUDA_INFO = [CUDA & GPU Aktif]
else
    CUDA_CFLAGS =
    CUDA_LDFLAGS =
    CUDA_OBJS =
    CUDA_INFO = [CPU Modu]
endif

CFLAGS ?= -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L $(PKG_CFLAGS) $(CUDA_CFLAGS)
LDFLAGS ?= -lm $(PKG_LIBS) $(CUDA_LDFLAGS)

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/cicekliscript

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/lexer.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/ast.c \
       $(SRC_DIR)/value.c \
       $(SRC_DIR)/env.c \
       $(SRC_DIR)/gc.c \
       $(SRC_DIR)/eval.c \
       $(SRC_DIR)/gui.c \
       $(SRC_DIR)/tensor.c

OBJS = $(SRCS:.c=.o) $(CUDA_OBJS)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@ln -sf cicekliscript $(BIN_DIR)/ccl
	@ln -sf cicekliscript $(BIN_DIR)/cicekli
	@ln -sf cicekliscript $(BIN_DIR)/metescript
	@echo "CicekliScript başarıyla derlendi: $(TARGET)$(CUDA_INFO)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/gpu_cuda.o: $(SRC_DIR)/gpu_cuda.cu
	$(NVCC) -O3 -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET) $(BIN_DIR)/ccl $(BIN_DIR)/cicekli $(BIN_DIR)/metescript
	@echo "Temizlendi."

install: $(TARGET)
	install -d $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/cicekliscript
	ln -sf $(HOME)/.local/bin/cicekliscript $(HOME)/.local/bin/ccl
	ln -sf $(HOME)/.local/bin/cicekliscript $(HOME)/.local/bin/cicekli
	ln -sf $(HOME)/.local/bin/cicekliscript $(HOME)/.local/bin/metescript
	@if [ -d "$(HOME)/.gemini/antigravity/bin" ]; then \
		ln -sf $(HOME)/.local/bin/cicekliscript $(HOME)/.gemini/antigravity/bin/ccl; \
		ln -sf $(HOME)/.local/bin/cicekliscript $(HOME)/.gemini/antigravity/bin/cicekli; \
		ln -sf $(HOME)/.local/bin/cicekliscript $(HOME)/.gemini/antigravity/bin/cicekliscript; \
	fi
	@echo "CicekliScript ~/.local/bin/cicekliscript ve 'ccl' kısayolu başarıyla yüklendi."
