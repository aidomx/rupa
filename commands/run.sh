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

get_file_tests() {
  local -n tests_ref=$1

  tests_ref=()

  while IFS= read -r -d '' file; do
    tests_ref+=("$file")
  done < <(
    find "$APP_ROOT/tests" \
      -type f \
      -name '*.rp' \
      -print0 2>/dev/null |
      sort -z
  )

  if [[ ${#tests_ref[@]} -eq 0 ]]; then
    print_warning "No .rp files found in tests/"
    return 1
  fi
}

test_help() {
  cat <<EOF
Usage:
  DEV_MODE=1 ./build.sh test [options]

Options:
  --list              Show all available test files
  --select "1,3,7"    Run selected test files
  --help | -h         Show this help message

Examples:
  DEV_MODE=1 ./build.sh test
  DEV_MODE=1 ./build.sh test --list
  DEV_MODE=1 ./build.sh test --select "7,13"
EOF
}

test_lists() {
  local files=()
  local file
  local total
  local width=1
  local i=0

  get_file_tests files || return $?

  total="${#files[@]}"

  if [[ "$total" -eq 0 ]]; then
    print_warning "No .rp files found in tests/"
    return 0
  fi

  # Hitung lebar nomor terbesar
  width="${#total}"

  echo
  echo -e "${CYAN}> Syntax Tests${NC}"
  echo "  Found ${total} test files"
  echo

  printf "%-${width}s | %s\n" "#" "Filename"
  printf "%-${width}s-+-%s\n" \
    "$(printf '%*s' "$width" '' | tr ' ' '-')" \
    "$(printf '%*s' 40 '' | tr ' ' '-')"

  for file in "${files[@]}"; do
    i=$((i + 1))

    # Hilangkan APP_ROOT agar path lebih pendek
    file="${file#"$APP_ROOT/tests/"}"

    printf "%-${width}d | %s\n" "$i" "$file"
  done

  echo
}

select_file_test() {
  local selection="$1"
  local files=()
  local indexes=()
  local selected=()
  local index
  local file_index

  get_file_tests files || return $?

  IFS=',' read -ra indexes <<<"$selection"

  for index in "${indexes[@]}"; do
    # Pastikan hanya angka
    if ! [[ "$index" =~ ^[0-9]+$ ]]; then
      print_error "Invalid test index: $index"
      return 1
    fi

    # User melihat list dari 1, array dimulai dari 0
    file_index=$((index - 1))

    if ((file_index < 0 || file_index >= ${#files[@]})); then
      print_error "Test index out of range: $index"
      return 1
    fi

    selected+=("${files[$file_index]}")
  done

  echo "Selected tests:"

  for file in "${selected[@]}"; do
    echo "  ${file#"$APP_ROOT/tests/"}"
  done

  # Jalankan nanti
  "$TARGET" --test "${selected[@]}"
}

run_test() {
  ensure_test_binary || return $?

  local tests=()
  local arguments=("$@")
  local i

  for ((i = 0; i < ${#arguments[@]}; i++)); do
    case "${arguments[$i]}" in
    test) ;;

    --help | -h)
      test_help
      return 0
      ;;

    --list)
      test_lists
      return 0
      ;;

    --select)
      ((i++))

      if [[ -z "${arguments[$i]:-}" ]]; then
        print_error "Missing test selection"
        return 1
      fi

      select_file_test "${arguments[$i]}"
      return 0
      ;;
    esac
  done

  get_file_tests tests || return $?
  "$TARGET" --test "${tests[@]}"
}
