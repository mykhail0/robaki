CXX = g++ -std=c++17

DEBUG = -fstack-protector-strong -g
NDEBUG = -DNDEBUG

CXXFLAGS = -pedantic -Wall -Wextra -Werror -O2 $(DEBUG)

SRC_DIR = src
BUILD_DIR = build
TEST_BIN_DIR = test_bin

CLIENT_DIR = $(SRC_DIR)/client
SERVER_DIR = $(SRC_DIR)/server
TEST_DIR = $(SRC_DIR)/tests

VPATH = $(SRC_DIR) $(CLIENT_DIR) $(SERVER_DIR) $(TEST_DIR) .

.PHONY: all clean tests del_obj del_test_bin del_bin

OBJ = $(BUILD_DIR)/utility.o $(BUILD_DIR)/crc32.o $(BUILD_DIR)/error.o $(BUILD_DIR)/socket.o $(BUILD_DIR)/timer.o

CLIENT_OBJ = $(BUILD_DIR)/client_config.o \
$(BUILD_DIR)/client.o $(BUILD_DIR)/from_game.o $(BUILD_DIR)/events.o \
$(BUILD_DIR)/from_gui.o $(BUILD_DIR)/to_game.o $(BUILD_DIR)/iset.o $(BUILD_DIR)/runtime_exceptions.o

SERVER_OBJ = $(BUILD_DIR)/server.o \
$(BUILD_DIR)/game.o $(BUILD_DIR)/to_client.o $(BUILD_DIR)/from_client.o \
$(BUILD_DIR)/server_config.o $(BUILD_DIR)/rng.o

TEST_OBJ = $(BUILD_DIR)/iset_test.o $(BUILD_DIR)/endianness.o $(BUILD_DIR)/rand.o $(BUILD_DIR)/timer.o

BINS = $(BUILD_DIR)/screen-worms-client $(BUILD_DIR)/screen-worms-server

TEST_BINS = $(TEST_BIN_DIR)/iset_test $(TEST_BIN_DIR)/endianness $(TEST_BIN_DIR)/rand $(TEST_BIN_DIR)/timer

all: $(BINS)

tests: $(TEST_BINS)

# Cleaning up.
del_obj:
	rm -rf $(BUILD_DIR)
del_test_bin:
	rm -rf $(TEST_BIN_DIR)
del_bin:
	rm -f screen-worms-client screen-worms-server

clean: del_bin del_test_bin del_obj

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
$(TEST_BIN_DIR)/iset_test: $(BUILD_DIR)/iset_test.o $(BUILD_DIR)/iset.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_BIN_DIR)/endianness: $(BUILD_DIR)/endianness.o $(BUILD_DIR)/utility.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_BIN_DIR)/rand: $(BUILD_DIR)/rand.o $(BUILD_DIR)/rng.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_BIN_DIR)/timer: $(BUILD_DIR)/timer.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/utility.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Objects.
$(BUILD_DIR)/crc32.o: $(SRC_DIR)/crc32.cpp $(SRC_DIR)/crc32.h
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
$(BUILD_DIR)/server.o: $(SERVER_DIR)/server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/game.h $(SERVER_DIR)/from_client.h $(SRC_DIR)/timer.h
$(BUILD_DIR)/screen-worms-server.o: $(SERVER_DIR)/screen-worms-server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/config.h

# Test objects dependencies.
$(BUILD_DIR)/iset_test.o: $(TEST_DIR)/iset_test.cpp $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(BUILD_DIR)/endianness.o: $(TEST_DIR)/endianness.cpp $(SRC_DIR)/utility.h
$(BUILD_DIR)/rand.o: $(TEST_DIR)/rand.cpp $(SERVER_DIR)/rng.h
