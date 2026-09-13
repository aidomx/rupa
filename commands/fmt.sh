#!/usr/bin/env bash

fmt_help() {
  cat << EOF2
Usage:
  DEV_MODE=1 ./build.sh fmt - [options]

Options:
  -                   Run formatter tests for all .rp files in tests/
  --select "1,3,7"    Run selected formatter tests from tests/
  --help | -h         Show this help message

Examples:
  DEV_MODE=1 ./build.sh fmt -
  DEV_MODE=1 ./build.sh fmt - --select "1"
  DEV_MODE=1 ./build.sh fmt - --select "1,4,7"
EOF2
}

get_fmt_tests() {
  local -n tests_ref=$1
  tests_ref=()

  while IFS= read -r -d '' file; do
    tests_ref+=("$file")
  done < <(
    find "$APP_ROOT/tests" \
      -type f \
      -name '*.rp' \
      -print0 2>/dev/null | sort -z
  )

  if [[ ${#tests_ref[@]} -eq 0 ]]; then
    print_warning "No .rp files found in tests/"
    return 1
  fi
}

select_fmt_tests() {
  local selection="$1"
  local -n files_ref=$2
  local -n selected_ref=$3
  local indexes index file_index

  selected_ref=()
  IFS=',' read -ra indexes <<< "$selection"

  for index in "${indexes[@]}"; do
    if ! [[ "$index" =~ ^[0-9]+$ ]]; then
      print_error "Invalid fmt test index: $index"
      return 1
    fi

    file_index=$((index - 1))
    if ((file_index < 0 || file_index >= ${#files_ref[@]})); then
      print_error "Fmt test index out of range: $index"
      return 1
    fi

    selected_ref+=("${files_ref[$file_index]}")
  done
}

run_fmt_file() {
  local file="$1"
  "$TARGET" fmt - < "$file"
}

run_fmt_tests() {
  local files=() selected=() selection=""
  local i=0 total=0 failed=0 file status rel

  get_fmt_tests files || return $?
  total=${#files[@]}

  if [[ -n "$2" ]]; then
    selection="$2"
    select_fmt_tests "$selection" files selected || return $?
  else
    selected=("${files[@]}")
  fi

  total=${#selected[@]}
  echo -e "${CYAN}> Formatter Tests${NC}"
  echo "  Found $total test files"
  echo

  for file in "${selected[@]}"; do
    i=$((i + 1))
    rel="${file#"$APP_ROOT/"}"

    if run_fmt_file "$file" >/dev/null 2>&1; then
      status="${GREEN}PASS${NC}"
    else
      status="${RED}FAIL${NC}"
      failed=$((failed + 1))
    fi

    printf " %d/%d  %b | %s\n" "$i" "$total" "$status" "$rel"
  done

  echo
  if ((failed == 0)); then
    echo -e "${GREEN}> Formatter tests passed${NC}"
    return 0
  fi

  echo -e "${RED}> Formatter tests failed: $failed${NC}"
  return 1
}

run_fmt() {
  ensure_test_binary || return $?

  local arguments=("$@")
  local select=""
  local input=""
  local i

  for ((i = 0; i < ${#arguments[@]}; i++)); do
    case "${arguments[$i]}" in
      fmt) ;;
      -) input="-" ;;
      --help|-h)
        fmt_help
        return 0
        ;;
      --select)
        ((i++))
        if [[ -z "${arguments[$i]:-}" ]]; then
          print_error "Missing fmt test selection"
          return 1
        fi
        select="${arguments[$i]}"
        ;;
    esac
  done

  if [[ "$input" != "-" ]]; then
    print_error "Usage: ./build.sh fmt - [--select \"1,3\"]"
    return 1
  fi

  run_fmt_tests _ "$select"
}
