CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -g -Iinclude

SRC = source/main.c source/terminal.c source/buffer.c source/rope.c source/fileio.c
OBJ = $(SRC:source/%.c=build/%.o)

TARGET = build/bnano

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p build
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

build/%.o: source/%.c
	@mkdir -p build
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

run: all
	./$(TARGET)

.PHONY: all clean run
