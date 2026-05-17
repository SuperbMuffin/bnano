#!/bin/bash

set -e

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$TESTS_DIR")"
SRC_DIR="$ROOT_DIR/source"
INCLUDE_DIR="$ROOT_DIR/include"

GREEN="\033[0;32m"
RED="\033[0;31m"
CYAN="\033[0;36m"
BOLD="\033[1m"
RESET="\033[0m"

passed=0
failed=0

for test_file in "$TESTS_DIR"/*/test_*.c; do
  suite_dir="$(dirname "$test_file")"
  suite_name="$(basename "$suite_dir")"
  binary="$suite_dir/test_$suite_name"
  
  echo -e "${BOLD}${CYAN}=============================${RESET}"
  echo -e "${BOLD}${CYAN}==> $suite_name${RESET}"

  extra_sources=""
  if [ -f "$suite_dir/deps" ]; then
    while IFS= read -r dep; do
      extra_sources="$extra_sources $SRC_DIR/$dep"
    done < "$suite_dir/deps"
  fi

  if clang -Wall -Wextra \
      -I"$INCLUDE_DIR" \
      "$test_file" \
      "$SRC_DIR/$suite_name.c" \
      $extra_sources \
      -o "$binary" 2>&1; then
    if "$binary"; then
      echo -e "${GREEN}PASSED${RESET}"
      passed=$((passed + 1))
    else
      echo -e "${RED}FAILED${RESET}"
      failed=$((failed + 1))
    fi
  else
    echo -e "${RED}COMPILE FAILED${RESET}"
    failed=$((failed + 1))
  fi

  rm -f "$binary"
  echo ""
done

echo -e "${BOLD}-----------------------------${RESET}"
echo -e "suites passed: ${GREEN}${BOLD}$passed${RESET}"
echo -e "suites failed: ${RED}${BOLD}$failed${RESET}"

[ "$failed" -eq 0 ]
