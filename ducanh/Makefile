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
SERVER_OBJS = $(SERVER_DIR)/server.o \
              $(SERVER_DIR)/file_transfer.o \
              $(SERVER_DIR)/util.o \
              $(SERVER_DIR)/account.o \
              $(SERVER_DIR)/account_io.o \
              $(SERVER_DIR)/hash.o \
              $(SERVER_DIR)/session.o \
              $(SERVER_DIR)/config.o

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
	$(CC) $(CFLAGS) -pthread -o $@ $^

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
