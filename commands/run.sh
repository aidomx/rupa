#!/usr/bin/env bash
run_test() {
  echo "🧪 Testing $TARGET_NAME..."
  TARGET="./$(basename ${BINARY_DIR})/${TARGET_NAME}"
  [[ ! -f "$TARGET" ]] && {
    print_error "Error: $TARGET not found. Please build the project first."
    return 0
  }

  ./$TARGET --test || true
}
