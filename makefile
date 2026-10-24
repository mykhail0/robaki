# TODO remove C from the project, change FLAGS to CXXFLAGS
CC = gcc -std=c11
CXX = g++ -std=c++17

DEBUG = -fstack-protector-strong -g
NDEBUG = -DNDEBUG

FLAGS = -pedantic -Wall -Wextra -Werror -O2 $(DEBUG)

SRC_DIR = src
BUILD_DIR = build
TEST_BIN_DIR = test_bin

CLIENT_DIR = $(SRC_DIR)/client
SERVER_DIR = $(SRC_DIR)/server
TEST_DIR = $(SRC_DIR)/tests

VPATH = $(SRC_DIR) $(CLIENT_DIR) $(SERVER_DIR) $(TEST_DIR) .

.PHONY: all clean tests del_obj del_test_bin del_bin

OBJ = $(BUILD_DIR)/utility.o $(BUILD_DIR)/crc32.o

CLIENT_OBJ = $(BUILD_DIR)/screen-worms-client.o $(BUILD_DIR)/client_config.o \
$(BUILD_DIR)/client.o $(BUILD_DIR)/from_game.o $(BUILD_DIR)/events.o \
$(BUILD_DIR)/from_gui.o $(BUILD_DIR)/to_game.o $(BUILD_DIR)/iset.o

SERVER_OBJ = $(BUILD_DIR)/screen-worms-server.o $(BUILD_DIR)/server.o \
$(BUILD_DIR)/game.o $(BUILD_DIR)/to_client.o $(BUILD_DIR)/from_client.o \
$(BUILD_DIR)/server_config.o $(BUILD_DIR)/generator.o

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
	$(CXX) $(FLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) -c $< -o $@

$(BUILD_DIR)/client_config.o: $(CLIENT_DIR)/config.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(FLAGS) -c $< -o $@

$(BUILD_DIR)/server_config.o: $(SERVER_DIR)/config.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(FLAGS) -c $< -o $@

# Binaries.
$(BUILD_DIR)/screen-worms-client: $(CLIENT_OBJ) $(OBJ) $(BUILD_DIR)/err.o
	$(CXX) $(FLAGS) $^ -o $@
$(BUILD_DIR)/screen-worms-server: $(SERVER_OBJ) $(OBJ) $(BUILD_DIR)/err.o
	$(CXX) $(FLAGS) $^ -o $@

# Test binaries.
$(TEST_BIN_DIR)/iset_test: $(BUILD_DIR)/iset_test.o $(BUILD_DIR)/iset.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(FLAGS) $^ -o $@

$(TEST_BIN_DIR)/endianness: $(BUILD_DIR)/endianness.o $(BUILD_DIR)/utility.o $(BUILD_DIR)/err.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(FLAGS) $^ -o $@

$(TEST_BIN_DIR)/rand: $(BUILD_DIR)/rand.o $(BUILD_DIR)/generator.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(FLAGS) $^ -o $@

$(TEST_BIN_DIR)/timer: $(BUILD_DIR)/timer.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/utility.o $(BUILD_DIR)/err.o
	mkdir -p $(TEST_BIN_DIR)
	$(CXX) $(FLAGS) $^ -o $@

# Objects.
$(BUILD_DIR)/err.o: $(SRC_DIR)/err.c $(SRC_DIR)/err.h
$(BUILD_DIR)/crc32.o: $(SRC_DIR)/crc32.cpp $(SRC_DIR)/crc32.h
$(BUILD_DIR)/utility.o: $(SRC_DIR)/utility.cpp $(SRC_DIR)/utility.h $(SRC_DIR)/consts.h $(SRC_DIR)/err.h

# Client objects dependencies.
$(BUILD_DIR)/iset.o: $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(BUILD_DIR)/to_game.o: $(CLIENT_DIR)/to_game.cpp $(CLIENT_DIR)/to_game.h $(SRC_DIR)/consts.h
$(BUILD_DIR)/client_config.o: $(CLIENT_DIR)/config.cpp $(CLIENT_DIR)/config.h $(SRC_DIR)/err.h $(SRC_DIR)/utility.h
$(BUILD_DIR)/from_gui.o: $(CLIENT_DIR)/from_gui.cpp $(CLIENT_DIR)/from_gui.h $(SRC_DIR)/consts.h $(SRC_DIR)/err.h
$(BUILD_DIR)/events.o: $(CLIENT_DIR)/events.cpp $(CLIENT_DIR)/events.h $(SRC_DIR)/utility.h $(SRC_DIR)/err.h
$(BUILD_DIR)/from_game.o: $(CLIENT_DIR)/from_game.cpp $(CLIENT_DIR)/from_game.h $(CLIENT_DIR)/iset.h $(CLIENT_DIR)/events.h
$(BUILD_DIR)/client.o: $(CLIENT_DIR)/client.cpp $(CLIENT_DIR)/client.h $(CLIENT_DIR)/from_game.h $(CLIENT_DIR)/from_gui.h $(CLIENT_DIR)/to_game.h
$(BUILD_DIR)/screen-worms-client.o: $(CLIENT_DIR)/screen-worms-client.cpp $(CLIENT_DIR)/client.h $(CLIENT_DIR)/config.h $(SRC_DIR)/err.h

# Server objects dependencies.
$(BUILD_DIR)/server_config.o: $(SERVER_DIR)/config.cpp $(SERVER_DIR)/config.h $(SRC_DIR)/consts.h $(SRC_DIR)/err.h $(SRC_DIR)/utility.h
$(BUILD_DIR)/generator.o: $(SERVER_DIR)/generator.cpp $(SERVER_DIR)/generator.h
$(BUILD_DIR)/from_client.o: $(SERVER_DIR)/from_client.cpp $(SERVER_DIR)/from_client.h $(SRC_DIR)/utility.h
$(BUILD_DIR)/to_client.o: $(SERVER_DIR)/to_client.cpp $(SERVER_DIR)/to_client.h $(SRC_DIR)/utility.h $(SRC_DIR)/crc32.h
$(BUILD_DIR)/game.o: $(SERVER_DIR)/game.cpp $(SERVER_DIR)/game.h $(SERVER_DIR)/to_client.h $(SERVER_DIR)/generator.h
$(BUILD_DIR)/server.o: $(SERVER_DIR)/server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/game.h $(SERVER_DIR)/from_client.h
$(BUILD_DIR)/screen-worms-server.o: $(SERVER_DIR)/screen-worms-server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/config.h $(SRC_DIR)/err.h

# Test objects dependencies.
$(BUILD_DIR)/iset_test.o: $(TEST_DIR)/iset_test.cpp $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(BUILD_DIR)/endianness.o: $(TEST_DIR)/endianness.cpp $(SRC_DIR)/utility.h
$(BUILD_DIR)/rand.o: $(TEST_DIR)/rand.cpp $(SERVER_DIR)/generator.h
$(BUILD_DIR)/timer.o: $(TEST_DIR)/timer.cpp $(SRC_DIR)/utility.h
