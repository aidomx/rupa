# Compiler flags
WALL_FLAGS ?= -Wall -Wextra
INCLUDE_FLAGS ?= -Ibase -Idefs -Iinclude -I.
CC ?= gcc
CFLAGS ?= $(WALL_FLAGS) $(INCLUDE_FLAGS)
LDFLAGS ?=
export CC CFLAGS LDFLAGS

# Header
BUILD_HEADER ?= > Build artifacts
CLEAN_HEADER ?= > Cleaning build artifacts
DEBUG_HEADER ?= > Build artifacts with debugging
export BUILD_HEADER CLEAN_HEADER DEBUG_HEADER

# Directory setup
APP_DIR ?=
COMMAND_DIR ?= commands
COMMAND_FILES ?= $(shell find $(COMMAND_DIR) -name "*.sh")
LIB_DIR ?= lib
SRC_DIR ?= src
OBJ_DIR ?= build
BINARY_DIR ?= bin
TARGET ?= rupa
TARGET_BIN ?= $(BINARY_DIR)/$(TARGET)
TARGET_NAME ?= Rupa
TARGET_RELEASE_VERSION ?= 1.0
export COMMAND_DIR COMMAND_FILES LIB_DIR SRC_DIR OBJ_DIR BINARY_DIR TARGET TARGET_BIN TARGET_NAME TARGET_RELEASE_VERSION

# Source files
SRC ?= $(shell find $(SRC_DIR) -name "*.c")
OBJ ?= $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
export SRC OBJ

# Progress tracking
TOTAL_FILES ?= $(words $(SRC))
COMPILED_FILES ?= 0
COMPILED_COMMANDS ?= compile_commands.json
PERCENT ?= 0
export TOTAL_FILES COMPILED_FILES COMPILED_COMMANDS PERCENT

# Build flags - FIXED untuk aarch64
DEBUG_FLAGS ?= -g -O0
RELEASE_FLAGS ?= -g -O2 -DNDEBUG
LEAK_FLAGS ?= -fsanitize=leak
UB_FLAGS ?= -fsanitize=undefined
export DEBUG_FLAGS RELEASE_FLAGS LEAK_FLAGS UB_FLAGS
