// parser.h

#ifndef POP3_PARSER_H
#define POP3_PARSER_H

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include "logger.h"

// Add any necessary includes or declarations here

// Declaration for the function
void parseCommand(uint8_t * command, Logger* logger);

#endif  // POP3_PARSER_H
