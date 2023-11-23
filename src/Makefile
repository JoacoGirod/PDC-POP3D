CFLAGS = -std=c11 -pedantic -pedantic-errors -g -Wall -Werror -Wextra -Wno-unused-parameter -Wno-newline-eof -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200112L -fsanitize=address


SRCS = $(wildcard *.c) $(wildcard */*.c) $(wildcard */*/*.c)
OBJS = $(SRCS:.c=.o)

INCLUDE_DIRS := . ./parser/parser_automaton ./parser ./test
INCLUDES_DECLARATION := $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

SERVER_NAME = ../pop3d
LOGS = logs

all: $(SERVER_NAME)


$(SERVER_NAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES_DECLARATION) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES_DECLARATION) -c $< -o $@

clean:
	rm -rf $(OBJS)
	rm -f */*.o

.PHONY: clean all
