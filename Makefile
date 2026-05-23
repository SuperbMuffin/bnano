CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -g -Iinclude -MMD -MP
SRC = source/main.c source/terminal.c source/buffer.c source/rope.c source/fileio.c source/ui.c source/command.c source/history.c source/config.c
OBJ = $(SRC:source/%.c=build/%.o)
DEP = $(OBJ:.o=.d)

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
-include $(DEP)

clean:
	rm -rf build

run: all
	./$(TARGET)
.PHONY: all clean run
