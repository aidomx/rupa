#!/usr/bin/env bash
declare -gA UNUSED_FILES=()
declare -gA ERROR_FILES=()

build_clean() {
  rm -rf $BUILD_DIR $TARGET >/dev/null 2>&1 &
  PID=$!
  progress "dots" $PID "$CLEAN_HEADER"
  wait $PID
}

build_common() {
  local SKIPPED_FILES=0
  local NEEDS_COMPILE=()

  # Phase 1: determine which files need recompilation
  for file in "${SRC[@]}"; do
    local obj_file
    obj_file=$(echo "$file" | sed "s|^$SRC_DIR/|$BUILD_DIR/|" | sed 's|\.c$|.o|')

    if [[ -f "$obj_file" ]] && [[ "$obj_file" -nt "$file" ]]; then
      SKIPPED_FILES=$((SKIPPED_FILES + 1))
    else
      NEEDS_COMPILE+=("$file")
    fi
  done

  local COMPILE_TOTAL=${#NEEDS_COMPILE[@]}
  TOTAL_FILES=$COMPILE_TOTAL

  if [ "$COMPILE_TOTAL" -eq 0 ]; then
    echo -e "${CYAN}> All files up-to-date, nothing to compile.${NC}"
    link_executable
    local size=$(stat -c%s "$TARGET" 2>/dev/null | numfmt --to=iec 2>/dev/null || echo "unknown")
    echo ""
    echo -e "${CYAN}> Summary${NC}"
    echo -e "Target   : ${YELLOW}$TARGET${NC}"
    echo -e "Size     : ${YELLOW}$size${NC}"
    echo -e "Skipped  : ${YELLOW}$SKIPPED_FILES${NC} (up-to-date)"
    echo -e "Compiled : ${YELLOW}0${NC}"
    echo -e "Status   : ${GREEN}Success${NC}"
    echo -e "Errors   : 0"
    echo -e "Warnings : ${UNUSED_TOTAL:-0}"
    echo ""
    return 0
  fi

  # Phase 2: compile only changed files
  for file in "${NEEDS_COMPILE[@]}"; do
    COMPILED_FILES=$((COMPILED_FILES + 1))
    SOURCE="${file#"$APP_ROOT"/}"
    [[ "$SOURCE" == "$file" ]] && SOURCE="${file#"$SRC_DIR"/}"

    start=$(date +%s.%N)
    if output=$(compile_file "$file" 2>&1); then
      COMPILE_STATUS=0
    else
      COMPILE_STATUS=$?
    fi
    end=$(date +%s.%N)
    TIME=$(awk "BEGIN {printf \"%.2fs\", $end - $start}")

    if [ "$COMPILE_STATUS" -ne 0 ]; then
      STATUS="${RED}FAIL${NC}"
      SIZE="-"
      ERROR_TOTAL=$((ERROR_TOTAL + 1))
      printf "%s\n" "$output"
    else
      STATUS="${GREEN}OK${NC}"
      if [[ -f "$output" ]]; then
        bytes=$(stat -c%s "$output")
        SIZE="$((bytes / 1024))KB"
      else
        SIZE="-"
      fi
    fi

    _render_build_progress "$COMPILED_FILES" "$COMPILE_TOTAL" "$STATUS" "$SOURCE" "$SIZE" "$TIME"
    report_compile "$file"
  done

  scan_results

  if [ "$ERROR_TOTAL" -eq 0 ]; then
    link_executable
  else
    echo -e "${RED}Build failed; linking skipped.${NC}"
    return 1
  fi

  local size=$(stat -c%s "$TARGET" 2>/dev/null | numfmt --to=iec 2>/dev/null || echo "unknown")
  echo ""
  echo -e "${CYAN}> Summary${NC}"
  echo -e "Target   : ${YELLOW}$TARGET${NC}"
  echo -e "Size     : ${YELLOW}$size${NC}"
  echo -e "Skipped  : ${YELLOW}${SKIPPED_FILES:-0}${NC} (up-to-date)"
  echo -e "Compiled : ${YELLOW}$COMPILE_TOTAL${NC}"
  echo -e "Status   : ${GREEN}Success${NC}"
  echo -e "Errors   : ${ERROR_TOTAL:-0}"
  echo -e "Warnings : ${UNUSED_TOTAL:-0}"
  echo ""

  show_detail
  rm -rf "$LOG_DIR"
}

report_compile() {
  local file="$1"

  if [[ -d "$LOG_DIR" ]]; then
    file="$(basename "$file").log"

    if [[ -f "$LOG_DIR/$file" ]]; then
      log=$(cat "$LOG_DIR/$file")
      [[ -n "$log" ]] && echo "$log"
    fi
  fi
  return 0
}

_render_build_progress() {
  local current=$1 total=$2 status_str=$3 file=$4 size=$5 time=$6

  echo -e "Index    : $current dari $total"
  echo -e "Source   : $file"
  echo -e "Size     : $size"
  echo -e "Time     : $time"
  echo -e "Status   : $status_str"
  local width=${COLUMNS:-$(tput cols 2>/dev/null || echo 80)}
  printf '%*s\n' "$width" '' | tr ' ' '-'
}

compile_file() {
  local src_file=$1
  local obj_file
  local log_file

  if ! [ -f "$src_file" ]; then
    echo -e "${RED}No such file${NC}" >&2
    return 1
  fi

  obj_file=$(echo "$src_file" | sed "s|^$SRC_DIR/|$BUILD_DIR/|" | sed 's|\.c$|.o|')
  log_file="$LOG_DIR/$(basename "$src_file").log"

  mkdir -p "$(dirname "$obj_file")"
  [[ ! -d "$LOG_DIR" ]] && mkdir -p "$LOG_DIR"

  local output
  output=$($CC $CFLAGS -c "$src_file" -o "$obj_file" 2>&1)
  local status=$?

  # Always save log for scan_results
  echo "$output" >"$log_file"

  if [ $status -ne 0 ]; then
    echo "$output" >&2
    return 1
  fi

  echo "$obj_file"
}

scan_results() {
  [[ ! -d "$LOG_DIR" ]] && return 0

  for log in "$LOG_DIR"/*.log; do
    [ -f "$log" ] || continue

    file=$(basename "$log" .log)

    local unused
    unused=$(grep -c "unused" "$log" 2>/dev/null) || unused=0

    UNUSED_TOTAL=$((UNUSED_TOTAL + unused))
    if [ "$unused" -gt 0 ]; then
      UNUSED_FILES["$file"]=$unused
    fi
  done
  return 0
}

prepare_module_archive() {
  local archive="$APP_ROOT/modules/rupa_modules.tar.gz"
  local module_object="$BUILD_DIR/rupa_modules.o"

  [[ ! -d "$APP_ROOT/tests/modules" ]] && return 0
  mkdir -p "$APP_ROOT/modules" "$BUILD_DIR"
  tar czf "$archive" -C "$APP_ROOT/tests/modules" .
  (cd "$APP_ROOT" && ld -r -b binary -o "$module_object" "modules/rupa_modules.tar.gz")
}

link_executable() {
  [[ ! -d "$BINARY_DIR" ]] && mkdir -p "$BINARY_DIR"
  prepare_module_archive || return 1
  echo -e "${CYAN}> Linked${NC}"
  $CC find $BUILD_DIR -name "*.o" $LDFLAGS -o $TARGET >/dev/null 2>&1 &
  pid=$!
  progress "percent" $pid "[$TOTAL_FILES/$TOTAL_FILES] ${YELLOW}$TARGET...${NC}"
}

show_detail() {
  if $DEBUGGING; then
    echo -e "${CYAN}> Detail debugging${NC}"

    if [ "$UNUSED_TOTAL" -eq 0 ] && [ "$ERROR_TOTAL" -eq 0 ]; then
      echo "(no issues found)"
      return
    fi

    if [ "$ERROR_TOTAL" -gt 0 ]; then
      echo
      echo "[error]"
      for f in "${!ERROR_FILES[@]}"; do
        echo "- $f (${ERROR_FILES[$f]})"
      done
    fi

    if [ "$UNUSED_TOTAL" -gt 0 ]; then
      echo
      echo "[unused]"
      for f in "${!UNUSED_FILES[@]}"; do
        echo "- $f (${UNUSED_FILES[$f]})"
      done
    fi
  fi
}
