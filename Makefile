
SERVER_DIRECTORY = ./src
TARGET_DIRECTORY = ./bin
LOG_DIRECTORY = ./logs
SERVER_NAME = pop3d
all: server

server:
	cd $(SERVER_DIRECTORY); make all

clean:
	cd $(SERVER_DIRECTORY); make clean
	rm -f $(SERVER_NAME)
	rm -f $(LOG_DIRECTORY)/*.log

.PHONY: all clean server