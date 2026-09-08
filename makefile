CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -MMD -MP
SRCS = $(wildcard *.c) $(wildcard include/*.c) $(wildcard vendor/*.c)
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
