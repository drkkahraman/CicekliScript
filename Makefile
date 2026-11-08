CC ?= gcc
PKG_CFLAGS = $(shell pkg-config --cflags cairo x11)
PKG_LIBS = $(shell pkg-config --libs cairo x11)

CFLAGS ?= -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L $(PKG_CFLAGS)
LDFLAGS ?= -lm $(PKG_LIBS)

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/metescript

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/lexer.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/ast.c \
       $(SRC_DIR)/value.c \
       $(SRC_DIR)/env.c \
       $(SRC_DIR)/gc.c \
       $(SRC_DIR)/eval.c \
       $(SRC_DIR)/gui.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "MeteScript başarıyla derlendi: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
	@echo "Temizlendi."

install: $(TARGET)
	install -d $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/metescript
	@echo "MeteScript ~/.local/bin/metescript konumuna yüklendi."
