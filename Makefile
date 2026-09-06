CC = gcc
WALL_FLAGS = -Wall -Wextra -xc
HEADER_FLAGS = -Iinclude -I.
STD_FLAGS = -std=gnu11
CFLAGS = $(HEADER_FLAGS) $(WALL_FLAGS) $(STD_FLAGS)
SRC_DIR = src
OBJ_DIR = build
TARGET = bin/rupa
MODULE_ARCHIVE = modules/rupa_modules.tar.gz
MODULE_SOURCES = $(shell find tests/modules -type f -name "*.rp")
MODULE_OBJ = $(OBJ_DIR)/rupa_modules.o

SRC = $(shell find $(SRC_DIR) -type f -name "*.c")
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(TARGET): $(OBJ) $(MODULE_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) $(MODULE_OBJ) -o $@ -lm

$(MODULE_ARCHIVE): $(MODULE_SOURCES)
	@mkdir -p $(dir $@)
	tar czf $@ -C tests/modules .

$(MODULE_OBJ): $(MODULE_ARCHIVE)
	@mkdir -p $(dir $@)
	ld -r -b binary -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR) $(TARGET)
