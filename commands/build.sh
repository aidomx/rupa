#!/usr/bin/env bash
build_clean() {
  rm -rf $BUILD_DIR $TARGET >/dev/null 2>&1 &
  PID=$!
  progress "dots" $PID "$CLEAN_HEADER"
  wait $PID
}

build_common() {
  printf "%-4s %-20s %-8s %-8s %-8s\n" "#" "source" "time" "size" "status"

  local STATUS="?"

  for file in "${SRC[@]}"; do
    COMPILED_FILES=$((COMPILED_FILES + 1))
    SOURCE="${file##*${APP_NAME}-v${TARGET_RELEASE_VERSION}}"

    start=$(date +%s.%N)
    output=$(compile_file "$file" 2>&1)
    end=$(date +%s.%N)

    TIME=$(awk "BEGIN {printf \"%.2fs\", $end - $start}")

    if [[ -z "$output" ]]; then
      STATUS="FAIL"
      SIZE="-"
    else
      STATUS="OK"
      if [[ -f "$output" ]]; then
        bytes=$(stat -c%s "$output")
        SIZE="$((bytes / 1024))KB"
      else
        SIZE="-"
      fi
    fi

    # POTONG source biar rapi
    SRC_SHORT=$(basename "$SOURCE")
    SRC_SHORT=${SRC_SHORT:0:20}

    printf "%-4d %-20s %-8s %-8s %-8s\n" \
      "$COMPILED_FILES" "$SRC_SHORT" "$TIME" "$SIZE" "$STATUS"

    # ===== BARIS FIX =====
    printf "\033[K${CYAN}(%d/%d)${NC} %s\r" \
      "$COMPILED_FILES" "$TOTAL_FILES" "$SOURCE"
  done

  # newline terakhir biar prompt ga nempel
  printf "\n"
  #for file in "${SRC[@]}"; do
  #COMPILED_FILES=$((COMPILED_FILES + 1))

  #printf "\r\033[K${CYAN}[%2d/%2d]${NC} %-30s" $COMPILED_FILES $TOTAL_FILES "${file##*${APP_NAME}-v${TARGET_RELEASE_VERSION}}"
  ## Move to column 55 for percentage
  #printf "\033[55G%3d%%" $PERCENT

  #local result=$(compile_file "$file" 2>&1)

  ## Compile file
  #if [[ -n "$result" ]]; then
  #local end_time=$(date +%s.%N)
  #if [ -f "$result" ]; then
  #(stat -c%s "$result") >/dev/null 2>&1 &
  #local PID=$!
  #local duration=0
  #local max_duration=60

  #while [[ "$PERCENT" -lt 100 ]]; do
  #PERCENT=$((duration * 100 / max_duration))
  #printf "\r\033[55G%3d%%" $PERCENT
  #duration=$((duration + 1))
  #done
  #wait $PID

  #printf "\r\033[55G\033[K" # Clear percentage
  #printf "\033[55G${GREEN}✓${NC}\n"
  #PERCENT=0

  ## Simulate progress (karena compile cepat)
  ##for i in $(seq 0 25 100); do
  ##printf "\r\033[55G%3d%%" $i
  ##sleep 0.01
  ##done
  #fi
  #else
  #printf "\r\033[55G\033[K"
  #printf "\033[55G${RED}✗${NC}\n"
  #return 1
  #fi
  #done

  # Link
  link_executable

  # Show result
  local size=$(stat -c%s "$TARGET" 2>/dev/null | numfmt --to=iec 2>/dev/null || echo "unknown")
  echo ""
  echo -e "${CYAN}> Summary${NC}"
  echo -e "Target   : ${YELLOW}$TARGET${NC}"
  echo -e "Size     : ${YELLOW}$size${NC}"
  echo -e "Files    : ${YELLOW}$TOTAL_FILES${NC}"

  if [ "$ERROR_TOTAL" -gt 0 ]; then
    echo -e "Status   : ${RED}Error${NC}"
  else
    echo -e "Status   : ${GREEN}Success${NC}"
  fi

  echo -e "Errors   : ${ERROR_TOTAL:-0}"
  echo -e "Warnings : ${UNUSED_TOTAL:-0}"
  echo ""

  show_detail
  rm -rf "$LOG_DIR"
}

compile_file() {
  local src_file=$1

  if ! [ -f "$src_file" ]; then
    echo -e "${RED}No such file${NC}" >&2
    return 1
  fi

  local obj_file=$(echo "$src_file" | sed "s|^$SRC_DIR/|$BUILD_DIR/|" | sed 's|\.c$|.o|')
  local log_file="$LOG_DIR/$(basename "$src_file").log"

  mkdir -p "$(dirname "$obj_file")"
  [[ ! -d "$LOG_DIR" ]] && mkdir -p "$LOG_DIR"

  local output
  output=$($CC $CFLAGS -c "$src_file" -o "$obj_file" 2>&1)
  local status=$?

  # Simpan log hanya kalau debug
  if $DEBUGGING; then
    echo "$output" >"$log_file"
  fi

  if [ $status -ne 0 ]; then
    echo "$output" >&2
    return 1
  fi

  echo "$obj_file"
}

scan_results() {
  declare -gA UNUSED_FILES
  declare -gA ERROR_FILES

  for log in "$LOG_DIR"/*.log; do
    [ -f "$log" ] || continue

    local file=$(basename "$log" .log)

    local unused
    unused=$(grep -c "unused" "$log")

    local errors
    errors=$(grep -c "error:" "$log")

    ((UNUSED_TOTAL += unused))
    ((ERROR_TOTAL += errors))

    ((unused > 0)) && UNUSED_FILES["$file"]=$unused
    ((errors > 0)) && ERROR_FILES["$file"]=$errors
  done
}

link_executable() {
  echo -e "[$TOTAL_FILES/$TOTAL_FILES] ${YELLOW}Linking $TARGET...${NC}"
  $CC $(find $BUILD_DIR -name "*.o") -o $TARGET 2>&1
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
