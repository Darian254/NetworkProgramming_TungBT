CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := 

SRCS := main.c server.c epoll_loop.c connect.c router.c command.c
HDRS := server.h epoll.h config.h connect.h router.h command.h models.h
OBJS := $(SRCS:.c=.o)

TARGET := server
CLIENT := client

.PHONY: all clean run client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

client: client.c config.h
	$(CC) $(CFLAGS) -o $(CLIENT) client.c

clean:
	rm -f $(OBJS) $(TARGET) $(CLIENT)
