#!/usr/bin/env bash
build_clean() {
  rm -rf $BUILD_DIR $TARGET >/dev/null 2>&1 &
  PID=$!
  progress "dots" $PID "$CLEAN_HEADER"
  wait $PID
}

build_common() {
  local width=${COLUMNS:-$(tput cols 2>/dev/null || echo 80)}
  local compact=false
  (( width < 64 )) && compact=true

  # The progress line is temporary and always remains directly below the
  # completed source rows. Before printing a new row, erase it and move the
  # cursor one line up so the new row is inserted above the progress line.
  local progress_active=false

  printf "# | source"
  if ! $compact; then
    printf " | time   | size | status"
  else
    printf " | status"
  fi
  printf "\n"

  for file in "${SRC[@]}"; do
    if $progress_active; then
      # Progress occupies the last line. Insert a new source row immediately
      # above it so the progress line stays fixed at the bottom.
      printf '\033[2K\r\033[1L'
    fi

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
      STATUS="FAIL"
      SIZE="-"
      ERROR_TOTAL=$((ERROR_TOTAL + 1))
      printf "%s\n" "$output"
    else
      STATUS="OK"
      if [[ -f "$output" ]]; then
        bytes=$(stat -c%s "$output")
        SIZE="$((bytes / 1024))KB"
      else
        SIZE="-"
      fi
    fi

    # Width belongs to the whole row. Keep # and status fixed and give the
    # remaining space to source. Long paths are shortened from the left.
    if $compact; then
      local fixed=$((4 + 8)) # index + separators/status
      local max_source=$((width - fixed))
      (( max_source < 10 )) && max_source=10
      if (( ${#SOURCE} > max_source )); then
        SOURCE="...${SOURCE: -$((max_source - 3))}"
      fi
      printf "%d | %-*s | %s\n" "$COMPILED_FILES" "$max_source" "$SOURCE" "$STATUS"
    else
      local max_source=$((width - 31))
      (( max_source < 12 )) && max_source=12
      if (( ${#SOURCE} > max_source )); then
        SOURCE="...${SOURCE: -$((max_source - 3))}"
      fi
      printf "%d | %-*s | %-6s | %-4s | %s\n" \
        "$COMPILED_FILES" "$max_source" "$SOURCE" "$TIME" "$SIZE" "$STATUS"
    fi

    PERCENT=$((COMPILED_FILES * 100 / TOTAL_FILES))
    printf "(%d/%d) processing... %d%%" "$COMPILED_FILES" "$TOTAL_FILES" "$PERCENT"
    progress_active=true
  done

  if $progress_active; then
    # Remove the final progress line before linking/summary output.
    printf '\033[2K\r'
    printf '\n'
  fi

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
  echo -e "Files    : ${YELLOW}$TOTAL_FILES${NC}"
  echo -e "Status   : ${GREEN}Success${NC}"
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
