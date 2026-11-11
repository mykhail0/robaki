# TODO simplify some stuff, maybe prefixes to directories for some variables?
# TODO add building GUI into this

CXX = g++

DEBUG = -fstack-protector-strong -g
NDEBUG = -DNDEBUG

CXXFLAGS = -std=c++17 -pedantic -Wall -Wextra -Werror -O2 $(DEBUG)

SRC_DIR = src
BUILD_DIR = build

CLIENT_DIR = $(SRC_DIR)/client
SERVER_DIR = $(SRC_DIR)/server
TEST_DIR = $(SRC_DIR)/tests

VPATH = $(SRC_DIR) $(CLIENT_DIR) $(SERVER_DIR) $(TEST_DIR) .

.PHONY: all clean test

OBJ = $(BUILD_DIR)/utility.o $(BUILD_DIR)/crc32.o $(BUILD_DIR)/error.o $(BUILD_DIR)/socket.o $(BUILD_DIR)/timer.o

CLIENT_OBJ = $(BUILD_DIR)/client_config.o \
$(BUILD_DIR)/client.o $(BUILD_DIR)/from_game.o $(BUILD_DIR)/events.o \
$(BUILD_DIR)/from_gui.o $(BUILD_DIR)/to_game.o $(BUILD_DIR)/iset.o $(BUILD_DIR)/runtime_exceptions.o

SERVER_OBJ = $(BUILD_DIR)/server.o $(BUILD_DIR)/client_pool.o \
$(BUILD_DIR)/game.o $(BUILD_DIR)/to_client.o $(BUILD_DIR)/from_client.o \
$(BUILD_DIR)/server_config.o $(BUILD_DIR)/rng.o

TEST_OBJ = $(BUILD_DIR)/iset_test.o $(BUILD_DIR)/endianness.o

BINS = $(BUILD_DIR)/screen-worms-client $(BUILD_DIR)/screen-worms-server

TEST_BINS = $(BUILD_DIR)/iset_test $(BUILD_DIR)/endianness

all: $(BINS)

TEST_SUITE_P = tests/test_suite_p/tests.sh tests/test_suite_p/game_logic.py \
							 tests/test_suite_p/protocol.py tests/test_suite_p/game_launcher.py \
							 tests/test_suite_p/client_tests.py tests/test_suite_p/server_tests.py \
							 tests/test_suite_p/server_parameter_tests.py

TEST_SUITE_D = tests/test_suite_d/tests.sh tests/test_suite_d/test_config.ini \
							 tests/test_suite_d/communication.py tests/test_suite_d/tests_100.py \
							 tests/test_suite_d/tests_200.py tests/test_suite_d/tests_300.py

test: tests/test.sh $(TEST_SUITE_P) $(TEST_SUITE_D) $(BINS) $(TEST_BINS)
	$<

# Cleaning up.
clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/client_config.o: $(CLIENT_DIR)/config.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/server_config.o: $(SERVER_DIR)/config.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Binaries.
$(BUILD_DIR)/screen-worms-client: $(BUILD_DIR)/screen-worms-client.o $(OBJ) $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@
$(BUILD_DIR)/screen-worms-server: $(BUILD_DIR)/screen-worms-server.o $(OBJ) $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Test binaries.
$(BUILD_DIR)/iset_test: $(BUILD_DIR)/iset_test.o $(BUILD_DIR)/iset.o
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/endianness: $(BUILD_DIR)/endianness.o $(BUILD_DIR)/utility.o
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Objects.
$(BUILD_DIR)/crc32.o: $(SRC_DIR)/crc32.cpp $(SRC_DIR)/crc32.h $(SRC_DIR)/consts.h
$(BUILD_DIR)/error.o: $(SRC_DIR)/error.cpp $(SRC_DIR)/error.h
$(BUILD_DIR)/timer.o: $(SRC_DIR)/timer.cpp $(SRC_DIR)/timer.h $(SRC_DIR)/error.h
$(BUILD_DIR)/utility.o: $(SRC_DIR)/utility.cpp $(SRC_DIR)/utility.h $(SRC_DIR)/consts.h
$(BUILD_DIR)/socket.o: $(SRC_DIR)/socket.cpp $(SRC_DIR)/socket.h $(SRC_DIR)/utility.h

# Client objects dependencies.
$(BUILD_DIR)/iset.o: $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(BUILD_DIR)/iset.o: $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(BUILD_DIR)/to_game.o: $(CLIENT_DIR)/to_game.cpp $(CLIENT_DIR)/to_game.h $(SRC_DIR)/consts.h
$(BUILD_DIR)/client_config.o: $(CLIENT_DIR)/config.cpp $(CLIENT_DIR)/config.h $(SRC_DIR)/utility.h
$(BUILD_DIR)/from_gui.o: $(CLIENT_DIR)/from_gui.cpp $(CLIENT_DIR)/from_gui.h $(SRC_DIR)/consts.h $(SRC_DIR)/error.h $(SRC_DIR)/socket.h
$(BUILD_DIR)/events.o: $(CLIENT_DIR)/events.cpp $(CLIENT_DIR)/events.h $(SRC_DIR)/utility.h $(SRC_DIR)/error.h $(SRC_DIR)/socket.h $(CLIENT_DIR)/runtime_exceptions.h
$(BUILD_DIR)/from_game.o: $(CLIENT_DIR)/from_game.cpp $(CLIENT_DIR)/from_game.h $(CLIENT_DIR)/iset.h $(CLIENT_DIR)/events.h $(SRC_DIR)/socket.h
$(BUILD_DIR)/client.o: $(CLIENT_DIR)/client.cpp $(CLIENT_DIR)/client.h $(CLIENT_DIR)/from_game.h $(CLIENT_DIR)/from_gui.h $(CLIENT_DIR)/to_game.h $(SRC_DIR)/socket.h $(SRC_DIR)/utility.h $(SRC_DIR)/timer.h
$(BUILD_DIR)/screen-worms-client.o: $(CLIENT_DIR)/screen-worms-client.cpp $(CLIENT_DIR)/client.h $(CLIENT_DIR)/config.h $(SRC_DIR)/socket.h $(CLIENT_DIR)/runtime_exceptions.h

# Server objects dependencies.
$(BUILD_DIR)/server_config.o: $(SERVER_DIR)/config.cpp $(SERVER_DIR)/config.h $(SRC_DIR)/consts.h $(SRC_DIR)/utility.h
$(BUILD_DIR)/rng.o: $(SERVER_DIR)/rng.cpp $(SERVER_DIR)/rng.h
$(BUILD_DIR)/from_client.o: $(SERVER_DIR)/from_client.cpp $(SERVER_DIR)/from_client.h $(SRC_DIR)/utility.h
$(BUILD_DIR)/to_client.o: $(SERVER_DIR)/to_client.cpp $(SERVER_DIR)/to_client.h $(SRC_DIR)/utility.h $(SRC_DIR)/crc32.h
$(BUILD_DIR)/game.o: $(SERVER_DIR)/game.cpp $(SERVER_DIR)/game.h $(SERVER_DIR)/to_client.h $(SERVER_DIR)/rng.h
$(BUILD_DIR)/client_pool.o: $(SERVER_DIR)/client_pool.cpp $(SERVER_DIR)/client_pool.h $(SERVER_DIR)/game.h $(SRC_DIR)/socket.h
$(BUILD_DIR)/server.o: $(SERVER_DIR)/server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/game.h $(SERVER_DIR)/from_client.h $(SERVER_DIR)/config.h $(SERVER_DIR)/client_pool.h $(SRC_DIR)/timer.h $(SRC_DIR)/socket.h
$(BUILD_DIR)/screen-worms-server.o: $(SERVER_DIR)/screen-worms-server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/config.h

# Test objects dependencies.
$(BUILD_DIR)/iset_test.o: $(TEST_DIR)/iset_test.cpp $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(BUILD_DIR)/endianness.o: $(TEST_DIR)/endianness.cpp $(SRC_DIR)/utility.h
