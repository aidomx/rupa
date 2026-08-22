#!/usr/bin/env bash

build_test_binary() {
  local log_file
  log_file=$(mktemp "${TMPDIR:-/tmp}/rupa-build.XXXXXX")

  export DEBUGGING=false
  export CFLAGS="$CFLAGS $DEBUG_FLAGS $LEAK_FLAGS"

  # Build in the background so normal build output stays hidden. The user only
  # sees one progress line; compiler output is preserved for failures.
  (
    build_clean >/dev/null 2>&1
    build_common
  ) >"$log_file" 2>&1 &
  local pid=$!

  progress "percent" "$pid" "> Building rupa"

  if wait "$pid"; then
    rm -f "$log_file"
    return 0
  fi

  printf '\n'
  print_error "Build failed"
  cat "$log_file"
  rm -f "$log_file"
  return 1
}

ensure_test_binary() {
  TARGET="./$(basename "${BINARY_DIR}")/${TARGET_NAME}"

  [[ -f "$TARGET" ]] && return 0

  echo -e "${CYAN}> Check binary rupa${NC}"
  echo -e "${CYAN}>${NC} ${YELLOW}$TARGET${NC} tidak ditemukan!"
  local question="$(echo -e "${CYAN}>${NC} Build sekarang? [y/n] ")"
  read -p "$question" -i -N -r
  local answer="$REPLY"

  case "$answer" in
  y | Y | yes | YES)
    build_test_binary || return 1
    [[ -f "$TARGET" ]] || {
      print_error "Error: $TARGET tetap tidak ditemukan setelah build."
      return 1
    }
    ;;
  *)
    print_warning "Test dibatalkan"
    return 1
    ;;
  esac
}

run_test() {
  ensure_test_binary || return $?

  local tests=()
  while IFS= read -r -d '' file; do
    tests+=("$file")
  done < <(find "$APP_ROOT/tests" -type f -name '*.rp' -print0 2>/dev/null | sort -z)

  if [[ ${#tests[@]} -eq 0 ]]; then
    print_warning "No .rp files found in tests/"
    return 0
  fi

  "$TARGET" --test "${tests[@]}"
}
