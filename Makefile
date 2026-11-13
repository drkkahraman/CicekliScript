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
LDFLAGS ?= -lm -ldl $(PKG_LIBS) $(CUDA_LDFLAGS)

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
       $(SRC_DIR)/debug.c \
       $(SRC_DIR)/tensor.c

OBJS = $(SRCS:.c=.o) $(CUDA_OBJS)

.PHONY: all clean install test vsix

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

ZIG ?= $(HOME)/.local/zig/zig

vsix: $(TARGET)
	@mkdir -p vscode-extension/bin
	@cp $(TARGET) vscode-extension/bin/cicekliscript
	@strip -s vscode-extension/bin/cicekliscript
	@ln -sf cicekliscript vscode-extension/bin/ccl
	@if [ -x "$(ZIG)" ]; then \
		echo "Windows için cicekliscript.exe çapraz derleniyor (cross-compile)..."; \
		$(ZIG) cc -target x86_64-windows-gnu -O2 -std=c11 -DNO_X11 \
			src/main.c src/lexer.c src/parser.c src/ast.c src/value.c src/env.c src/gc.c src/eval.c src/debug.c src/tensor.c src/gui.c -lgdi32 \
			-o vscode-extension/bin/cicekliscript.exe; \
		cp vscode-extension/bin/cicekliscript.exe vscode-extension/bin/ccl.exe; \
		rm -f vscode-extension/bin/cicekliscript.pdb; \
	fi
	@if command -v vsce >/dev/null 2>&1; then \
		cd vscode-extension && vsce package --no-git-tag-version; \
	elif [ -x "$(HOME)/.gemini/antigravity/bin/vsce" ]; then \
		cd vscode-extension && $(HOME)/.gemini/antigravity/bin/vsce package --no-git-tag-version; \
	else \
		python3 vscode-extension/build_vsix.py; \
	fi
	@echo "✓ VS Code eklentisi (Linux + Windows PE) resmi vsce ile başarıyla paketlendi."
