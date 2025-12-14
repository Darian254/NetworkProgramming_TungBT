CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := 

SRCS := main.c server.c epoll_loop.c connect.c router.c command.c
HDRS := server.h epoll.h config.h connect.h router.h command.h models.h
OBJS := $(SRCS:.c=.o)

TARGET := server

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
