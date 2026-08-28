#!/usr/bin/env bash
build_release() {
  local target=${1:-"unix"}

  # Setup target-specific compiler and paths
  set_target_compiler "$target"
  set_target_path "$target"

  # Update target name with extension
  export TARGET="${BINARY_DIR}/${TARGET_NAME}${TARGET_EXT}"

  # Set release flags
  export CFLAGS="$CFLAGS $RELEASE_FLAGS"
  export DEBUGGING=false

  # Show header
  case "$target" in
  win32)
    echo -e "${CYAN}> Release build for Windows 32-bit${NC}"
    ;;
  win64)
    echo -e "${CYAN}> Release build for Windows 64-bit${NC}"
    ;;
  unix | *)
    echo -e "${CYAN}> Release build for Unix/Linux${NC}"
    ;;
  esac

  build_clean
  build_common
}
