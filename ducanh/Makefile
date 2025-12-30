# Compiler and flags
CC = gcc
# Thêm -pthread vào CFLAGS để đảm bảo cả lúc compile và link đều có
CFLAGS = -Wall -Wextra -std=c11 -O2 -pthread

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

# Server object files - Cleaned and merged for New Architecture
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
              $(SERVER_DIR)/hash.o \
              $(SERVER_DIR)/config.o \
              $(SERVER_DIR)/db.o \
              $(SERVER_DIR)/game_logic.o \
              $(SERVER_DIR)/chest_logic.o \
              $(SERVER_DIR)/challenge_logic.o \
              $(SERVER_DIR)/dc.o \
              $(SERVER_DIR)/ship_defs.o \
              $(SERVER_DIR)/networking.o \
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