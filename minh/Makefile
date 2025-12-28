# Compiler & flags
CC = gcc
CFLAGS = -Wall -g -I. -Iserver -Iplayer -Iteam -Ijoin -Iinvite -Ilog -Iutils -Iclient

# Server sources
SERVER_SRCS = main.c server/server.c protocol/protocol.c player/player.c team/team.c join/join.c invite/invite.c log/log.c utils/utils.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
SERVER_BIN = server/server

# Client sources
CLIENT_SRC = client/client.c
CLIENT_BIN = client/client

# Default build both
all: $(SERVER_BIN) $(CLIENT_BIN)

# Compile server
$(SERVER_BIN): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS)

# Compile client
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -pthread -o $@ $(CLIENT_SRC)

# Generic rule to compile .c -> .o (tạo thư mục nếu cần)
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJS) $(SERVER_BIN) $(CLIENT_BIN)
