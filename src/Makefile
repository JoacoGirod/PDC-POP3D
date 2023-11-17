CFLAGS = -std=c11 -pedantic -pedantic-errors -g -Wall -Werror -Wextra -Wno-unused-parameter -Wno-newline-eof -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200112L -fsanitize=address

OBJS = main.o buffer.o netutils.o logger.o pop3_parser.o

pop3d: $(OBJS)
	$(CC) $(CFLAGS) -o pop3d $(OBJS)

clean:
	rm -rf pop3d $(OBJS) logs
