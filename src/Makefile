CC = gcc -std=c11
CPP = g++ -std=c++17

DEBUG = -g
NDEBUG = -DNDEBUG

FLAGS = -Wall -Wextra -O2 $(DEBUG)

OBJ_DIR = obj
TEST_BIN_DIR = test_bin

CLIENT_DIR = client
SERVER_DIR = server
TEST_DIR = tests

.PHONY: all clean

OBJ = $(OBJ_DIR)/utility.o $(OBJ_DIR)/crc32.o

CLIENT_OBJ = $(OBJ_DIR)/screen-worms-client.o $(OBJ_DIR)/client_config.o \
$(OBJ_DIR)/client.o $(OBJ_DIR)/from_game.o $(OBJ_DIR)/events.o \
$(OBJ_DIR)/from_gui.o $(OBJ_DIR)/to_game.o $(OBJ_DIR)/iset.o

SERVER_OBJ = $(OBJ_DIR)/screen-worms-server.o $(OBJ_DIR)/server.o \
$(OBJ_DIR)/game.o $(OBJ_DIR)/to_client.o $(OBJ_DIR)/from_client.o \
$(OBJ_DIR)/server_config.o $(OBJ_DIR)/generator.o

TEST_OBJ = $(OBJ_DIR)/iset_test.o $(OBJ_DIR)/endianness.o $(OBJ_DIR)/rand.o $(OBJ_DIR)/timer.o

BINS = screen-worms-client screen-worms-server

TEST_BINS = $(TEST_BIN_DIR)/iset_test $(TEST_BIN_DIR)/endianness $(TEST_BIN_DIR)/rand $(TEST_BIN_DIR)/timer

all: $(BINS) $(SERVER_OBJ)

tests: $(TEST_BINS)

# Cleaning up.
del_obj:
	rm -rf $(OBJ_DIR)
del_test_bin:
	rm -rf $(TEST_BIN_DIR)
del_bin:
	rm -f screen-worms-client screen-worms-server

clean: del_bin del_test_bin del_obj

$(OBJ) $(CLIENT_OBJ) $(SERVER_OBJ) $(TEST_OBJ):
	mkdir -p $(OBJ_DIR)
	$(CPP) $(FLAGS) -c $< -o $@

$(BINS):
	$(CPP) $(FLAGS) $^ -o $@

$(TEST_BINS):
	mkdir -p $(TEST_BIN_DIR)
	$(CPP) $(FLAGS) $^ -o $@

# Objects.
$(OBJ_DIR)/err.o: err.c err.h
	mkdir -p $(OBJ_DIR)
	$(CC) $(FLAGS) -c $< -o $@
$(OBJ_DIR)/crc32.o: crc32.cpp crc32.h
$(OBJ_DIR)/utility.o: utility.cpp utility.h consts.h err.h

# Client objects dependencies.
$(OBJ_DIR)/iset.o: $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(OBJ_DIR)/to_game.o: $(CLIENT_DIR)/to_game.cpp $(CLIENT_DIR)/to_game.h consts.h
$(OBJ_DIR)/client_config.o: $(CLIENT_DIR)/config.cpp $(CLIENT_DIR)/config.h err.h utility.h
$(OBJ_DIR)/from_gui.o: $(CLIENT_DIR)/from_gui.cpp $(CLIENT_DIR)/from_gui.h consts.h err.h
$(OBJ_DIR)/events.o: $(CLIENT_DIR)/events.cpp $(CLIENT_DIR)/events.h utility.h err.h
$(OBJ_DIR)/from_game.o: $(CLIENT_DIR)/from_game.cpp $(CLIENT_DIR)/from_game.h $(CLIENT_DIR)/iset.h $(CLIENT_DIR)/events.h
$(OBJ_DIR)/client.o: $(CLIENT_DIR)/client.cpp $(CLIENT_DIR)/client.h $(CLIENT_DIR)/from_game.h $(CLIENT_DIR)/from_gui.h $(CLIENT_DIR)/to_game.h
$(OBJ_DIR)/screen-worms-client.o: $(CLIENT_DIR)/screen-worms-client.cpp $(CLIENT_DIR)/client.h $(CLIENT_DIR)/config.h err.h

# Server objects dependencies.
$(OBJ_DIR)/server_config.o: $(SERVER_DIR)/config.cpp $(SERVER_DIR)/config.h consts.h err.h utility.h
$(OBJ_DIR)/generator.o: $(SERVER_DIR)/generator.cpp $(SERVER_DIR)/generator.h
$(OBJ_DIR)/from_client.o: $(SERVER_DIR)/from_client.cpp $(SERVER_DIR)/from_client.h utility.h
$(OBJ_DIR)/to_client.o: $(SERVER_DIR)/to_client.cpp $(SERVER_DIR)/to_client.h utility.h crc32.h
$(OBJ_DIR)/game.o: $(SERVER_DIR)/game.cpp $(SERVER_DIR)/game.h $(SERVER_DIR)/to_client.h $(SERVER_DIR)/generator.h
$(OBJ_DIR)/server.o: $(SERVER_DIR)/server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/game.h $(SERVER_DIR)/from_client.h
$(OBJ_DIR)/screen-worms-server.o: $(SERVER_DIR)/screen-worms-server.cpp $(SERVER_DIR)/server.h $(SERVER_DIR)/config.h err.h

# Test objects dependencies.
$(OBJ_DIR)/iset_test.o: $(TEST_DIR)/iset_test.cpp $(CLIENT_DIR)/iset.cpp $(CLIENT_DIR)/iset.h
$(OBJ_DIR)/endianness.o: $(TEST_DIR)/endianness.cpp utility.h
$(OBJ_DIR)/rand.o: $(TEST_DIR)/rand.cpp $(SERVER_DIR)/generator.h
$(OBJ_DIR)/timer.o: $(TEST_DIR)/timer.cpp utility.h

# Test binaries depndencies.
$(TEST_BIN_DIR)/iset_test: $(OBJ_DIR)/iset_test.o $(OBJ_DIR)/iset.o
$(TEST_BIN_DIR)/endianness: $(OBJ_DIR)/endianness.o $(OBJ_DIR)/utility.o $(OBJ_DIR)/err.o
$(TEST_BIN_DIR)/rand: $(OBJ_DIR)/rand.o $(OBJ_DIR)/generator.o
$(TEST_BIN_DIR)/timer: $(OBJ_DIR)/timer.o $(OBJ_DIR)/timer.o $(OBJ_DIR)/utility.o $(OBJ_DIR)/err.o

# Binaries dependencies.
screen-worms-client: $(CLIENT_OBJ) $(OBJ) $(OBJ_DIR)/err.o
screen-worms-server: $(SERVER_OBJ) $(OBJ) $(OBJ_DIR)/err.o
