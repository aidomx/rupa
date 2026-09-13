#!/usr/bin/env bash

get_fmt_tests() {
  local -n tests_ref=$1
  tests_ref=()
  while IFS='' read -r -d '' file; do
    tests_ref+=("$file")
  done < <(
    find "$APP_ROOT/tests" -type f -name '*.rp' -print0 2>/dev/null | sort -z
  )
  if [[ ${#tests_ref[@]} -eq 0 ]]; then
    print_warning "No .rp files found in tests/"
    return 1
  fi
}

select_fmt_tests() {
  local -n all_ref=$1
  local -n selected_ref=$2
  local spec="$3"
  selected_ref=()
  local item idx
  IFS=',' read -ra items <<< "$spec"
  for item in "${items[@]}"; do
    item="${item//[[:space:]]/}"
    [[ "$item" =~ ^[0-9]+$ ]] || {
      print_warning "Invalid fmt selection: $item"
      return 1
    }
    idx=$((item - 1))
    if (( idx < 0 || idx >= ${#all_ref[@]} )); then
      print_warning "Fmt selection out of range: $item"
      return 1
    fi
    selected_ref+=("${all_ref[idx]}")
  done
}

run_fmt_file() {
  local file="$1"
  local output
  local status

  output="$(mktemp)"
  if "$TARGET" fmt - < "$file" >"$output"; then
    status=0
  else
    status=$?
  fi

  cat "$output"
  rm -f "$output"
  return "$status"
}

run_fmt_tests() {
  local -a files=("$@")
  local passed=0 failed=0 i=0 status
  local file

  echo
  echo -e "${CYAN}> Formatter Tests${NC}"
  echo "  Found ${#files[@]} test files"

  for file in "${files[@]}"; do
    i=$((i + 1))
    echo
    echo "--- $file ---"
    if run_fmt_file "$file"; then
      status=0
      passed=$((passed + 1))
      echo "PASS | $file"
    else
      status=$?
      failed=$((failed + 1))
      echo "FAIL | $file"
    fi
  done

  echo
  echo -e "${CYAN}> Formatter test summary${NC}"
  echo "Passed : $passed"
  echo "Failed : $failed"
  if (( failed == 0 )); then
    echo "Status : Success"
    return 0
  fi
  echo "Status : Failed"
  return 1
}

fmt_help() {
  cat <<EOF_HELP
Usage:
  DEV_MODE=1 ./build.sh fmt - [options]

Options:
  -                   Format test files under tests/
  --select "1,3,7"    Run selected formatter test files
  --help | -h         Show this help message
EOF_HELP
}

run_fmt() {
  local input="${1:-}"
  local select=""
  shift || true

  [[ "$input" == "fmt" ]] && { input="${1:-}"; shift || true; }

  if [[ "$input" == "--help" || "$input" == "-h" || -z "$input" ]]; then
    fmt_help
    return [[ -z "$input" ]] && 1 || 0
  fi
  if [[ "$input" != "-" ]]; then
    print_error "fmt test expects '-'"
    return 1
  fi

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --select)
        [[ $# -ge 2 ]] || { print_error "fmt: --select requires a value"; return 1; }
        select="$2"
        shift 2
        ;;
      --help|-h)
        fmt_help
        return 0
        ;;
      *)
        print_error "fmt: unknown option '$1'"
        return 1
        ;;
    esac
  done

  ensure_test_binary || return 1

  local -a all_files=() files=()
  get_fmt_tests all_files || return 1
  if [[ -n "$select" ]]; then
    select_fmt_tests all_files files "$select" || return 1
  else
    files=("${all_files[@]}")
  fi

  run_fmt_tests "${files[@]}"
}
