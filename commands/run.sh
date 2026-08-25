#!/usr/bin/env bash

export build="${CMD_DIR}/build_test.sh"

compile() {
  export compdb=false

  [[ -f $COMPDB_FILE ]] && export compdb=true

  if command -v intercept-build >/dev/null 2>&1; then
    intercept-build $build "$@"
  elif command -v bear >/dev/null 2>&1; then
    bear -- $build "$@"
  else
    . $build
  fi
  return 1

}

rebuild_binary() {
  local question
  local answer
  local message="$1"

  question="$(echo -e "${CYAN}>${NC} $message, rebuild now? [y/n] ")"

  if ! read -r -t 7 -p "$question" answer; then
    echo
    return 1
  fi

  case "$answer" in
  y | Y | yes | YES)
    compile || return 1

    [[ -f "$TARGET" ]] || {
      print_error "Error: $TARGET tetap tidak ditemukan setelah build."
      return 1
    }
    ;;
  *)
    return 1
    ;;
  esac
}

# Fungsi pembantu untuk mengecek apakah perlu rebuild
needs_rebuild() {
  # Cari file sumber terbaru di src (sesuaikan ekstensi sesuai kebutuhan, misal .c .cpp .h)
  local latest_src=$(find src -type f \( -name '*.c' \) -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -1 | cut -d' ' -f2-)
  if [[ -z "$latest_src" ]]; then
    # Tidak ada file sumber -> anggap perlu rebuild (atau bisa return 1)
    return 0
  fi
  # Bandingkan timestamp modifikasi file sumber terbaru dengan timestamp binary
  [[ "$latest_src" -nt "$TARGET" ]]
}

check_binary_runs() {
  local target="$1"

  if ! "$target" --version &>/dev/null; then
    local err_msg=$("$target" --version 2>&1)
    if [[ "$err_msg" == *"cannot execute"* ]] || [[ "$err_msg" == *"not found"* ]] &>/dev/null; then
      rebuild_binary "Source may be is corrupted"
      return 1
    fi
  fi
  return 0
}

# --- Helper untuk verifikasi binary ---
verify_binary() {
  if [[ ! -x "$TARGET" ]]; then
    print_error "Binary $TARGET tidak memiliki izin eksekusi."
    return 1
  fi
  # Cek dependensi (gunakan ldd jika ada)
  if command -v ldd &>/dev/null; then
    local missing=$(ldd "$TARGET" 2>/dev/null | grep -i "not found")
    if [[ -n "$missing" ]]; then
      print_error "Binary $TARGET membutuhkan library yang tidak ditemukan:"
      echo "$missing" | while read -r line; do
        echo -e "${RED}  $line${NC}"
      done
      return 1
    fi
  fi
  return 0
}

ensure_test_binary() {
  local question
  local answer
  TARGET="./$(basename "${BINARY_DIR}")/${TARGET_NAME}"

  if [[ -f "$TARGET" ]]; then
    # Binary ada, cek apakah perlu rebuild
    if needs_rebuild; then
      rebuild_binary "Source may be has updated"
      return 0
    fi

    if verify_binary; then
      check_binary_runs "$TARGET"
      return 0
    fi
    return 0
  fi

  # Binary tidak ada
  echo -e "${CYAN}> Check binary rupa${NC}"
  echo -e "${CYAN}>${NC} ${YELLOW}$TARGET${NC} tidak ditemukan!"

  question="$(echo -e "${CYAN}>${NC} Build sekarang? [y/n] ")"

  if ! read -r -t 7 -p "$question" answer; then
    echo
    compile || return 1
    [[ -f "$TARGET" ]] || {
      print_error "Error: $TARGET tetap tidak ditemukan setelah build."
      return 1
    }
    return 0
  fi

  case "$answer" in
  y | Y | yes | YES)
    compile || return 1
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

  echo -e "${CYAN}>${NC} Selected tests:"

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
