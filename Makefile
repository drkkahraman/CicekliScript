CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -lm

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
       $(SRC_DIR)/eval.c

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
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/metescript
	@echo "MeteScript /usr/local/bin/metescript konumuna yüklendi."
