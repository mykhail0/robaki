#!/bin/bash

clear
cp ../src/screen-worms-* .
# python3 server_tests.py ServerLogicTests.test_basic_gameplay_direction_changes 1> output.txt 2> error.txt
python3 tests_100.py
python3 tests_300.py
python3 tests_200.py
