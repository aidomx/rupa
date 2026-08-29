CC = gcc
WALL_FLAGS = -Wall -Wextra -xc
HEADER_FLAGS = -Iinclude -I.
STD_FLAGS = -std=gnu11
CFLAGS = $(HEADER_FLAGS) $(WALL_FLAGS) $(STD_FLAGS)
SRC_DIR = src
OBJ_DIR = build
TARGET = bin/rupa

SRC = $(shell find $(SRC_DIR) -type f -name "*.c")
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR) $(TARGET)
