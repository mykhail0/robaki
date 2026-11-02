#!/bin/bash

set -u

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
cd "$SCRIPT_DIR" || {
  echo "Error: Failed to change directory to $SCRIPT_DIR" >&2
  exit 1
}

build_dir="../../build"

cp "${build_dir}/screen-worms-client" .
cp "${build_dir}/screen-worms-server" .
python3 tests_100.py
python3 tests_200.py
python3 tests_300.py
rm screen-worms-*
