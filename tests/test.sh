#!/bin/bash

# Change to script directory
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
cd "$SCRIPT_DIR" || {
  echo "Error: Failed to change directory to $SCRIPT_DIR" >&2
  exit 1
}

# Run a binary executable and verify its success
run_binary() {
  local bin_path="$1"

  if [ ! -x "$bin_path" ]; then
    echo "Error: $bin_path binary not found or lacks execute permissions." >&2
    exit 1
  fi

  "$bin_path" || {
    local retval=$?
    echo "Test $bin_path failed with exit code $retval." >&2
    exit "$retval"
  }
}

# Run an executable test script
run_script() {
  local script_path="$1"

  if [ ! -x "$script_path" ]; then
    echo "Error: $script_path script not found or lacks execute permissions." >&2
    exit 1
  fi

  "$script_path"
}

# Execute binary tests
run_binary "../build/iset_test"
run_binary "../build/endianness"

# Execute test suite scripts
run_script "test_suite_d/tests.sh"
run_script "test_suite_p/tests.sh"
