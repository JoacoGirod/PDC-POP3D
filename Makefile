
SERVER_DIRECTORY = ./src
TARGET_DIRECTORY = ./bin
LOG_DIRECTORY = ./logs

all: server


server:
	cd $(SERVER_DIRECTORY); make all

clean:
	cd $(SERVER_DIRECTORY); make clean

.PHONY: all clean server