#!/bin/bash

clear
cp ../src/screen-worms-* .
# python3 server_tests.py ServerLogicTests.test_basic_gameplay_direction_changes 1> output.txt 2> error.txt
# python3 server_tests.py
python3 client_tests.py
