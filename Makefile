CFLAGS = -std=c11 -pedantic -pedantic-errors -g -Wall -Werror -Wextra -Wno-unused-parameter -Wno-newline-eof -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200112L -fsanitize=address

COMPILER = ${CC}
CFLAGS = --std=c11 -Wall -pedantic -pedantic-errors -Wformat=2 -Wextra -Wno-unused-parameter -Wundef -Wuninitialized -Wno-implicit-fallthrough -D_POSIX_C_SOURCE=200809L -fsanitize=address -g -O3

SERVER_NAME = popserver
ADMIN_NAME = popadmin

ADMIN_DIR = ./admin
SERVER_DIR = ./server

TARGET_DIR = ./bin
LOG_DIR = ./log

all: server admin

server:
	cd $(SERVER_DIR); make all
	mkdir -p $(TARGET_DIR)
	cp $(SERVER_DIR)/$(SERVER_NAME) $(TARGET_DIR)/$(SERVER_NAME)
	rm -f $(SERVER_DIR)/$(SERVER_NAME) 

admin:
	cd $(ADMIN_DIR); make all
	mkdir -p $(TARGET_DIR)
	cp $(ADMIN_DIR)/$(ADMIN_NAME) $(TARGET_DIR)/$(ADMIN_NAME)
	rm -f $(ADMIN_DIR)/$(ADMIN_NAME)

clean:
	rm -rf $(TARGET_DIR)
	rm -rf $(LOG_DIR)
	cd $(SERVER_DIR); make clean
	cd $(ADMIN_DIR); make clean

.PHONY = clean, all
