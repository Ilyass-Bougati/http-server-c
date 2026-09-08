CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -MMD -MP
SRCS = server.c include/handler.c include/response.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

all: server

server: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server $(OBJS) $(DEPS)

-include $(DEPS)

.PHONY: all clean
