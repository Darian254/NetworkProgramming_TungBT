# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

# Directories
CLIENT_DIR = TCP_Client
SERVER_DIR = TCP_Server

# Executables
CLIENT = client
SERVER = server

# Client object files
CLIENT_OBJS = $(CLIENT_DIR)/client.o \
              $(CLIENT_DIR)/ui.o \
              $(SERVER_DIR)/file_transfer.o \
              $(SERVER_DIR)/util.o \
              $(SERVER_DIR)/config.o

# Server object files
# TODO: This is the new modular architecture
# - server.o: Clean epoll-based server (replaces old server.o)
# - app_context.o: Global state management (user table, sessions)
# - router.o: Command routing layer
# - command.o: Command parsing
# - epoll_loop.o: Event loop from phu
# - connect.o: Connection management from phu
# - session.o: Business logic handlers (LOGIN, REGISTER, etc.) - UNCHANGED
# - users.o/users_io.o/hash.o: User management - UNCHANGED
# - db.o: Game logic (teams, matches, ships) - UNCHANGED
# - file_transfer.o/util.o/config.o: Utilities - UNCHANGED
#
# To use new architecture:
#   1. Change server_new.o to server.o below
#   2. Or keep both and compile: make server_new vs make server_old
SERVER_OBJS = $(SERVER_DIR)/server.o \
              $(SERVER_DIR)/app_context.o \
              $(SERVER_DIR)/router.o \
              $(SERVER_DIR)/command.o \
              $(SERVER_DIR)/epoll_loop.o \
              $(SERVER_DIR)/connect.o \
              $(SERVER_DIR)/session.o \
              $(SERVER_DIR)/file_transfer.o \
              $(SERVER_DIR)/util.o \
              $(SERVER_DIR)/users.o \
              $(SERVER_DIR)/users_io.o \
              $(SERVER_DIR)/config.o \
              $(SERVER_DIR)/hash.o \
              $(SERVER_DIR)/db.o \
              $(SERVER_DIR)/team_handler.o 

.PHONY: all clean client server

# ==============================
# Build all
# ==============================
all: $(CLIENT) $(SERVER)

# ==============================
# Build client
# ==============================
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# ==============================
# Build server
# ==============================
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# ==============================
# Compilation rules
# ==============================
$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==============================
# Clean
# ==============================
clean:
	rm -f $(CLIENT) $(SERVER) $(CLIENT_DIR)/*.o $(SERVER_DIR)/*.o

# ==============================
# Run
# ==============================
run_server: $(SERVER)
	./$(SERVER) 5500

run_client: $(CLIENT)
	./$(CLIENT) 127.0.0.1 5500
