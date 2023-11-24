#ifndef __POP3_PARSER_ADT_H__
#define __POP3_PARSER_ADT_H__

#include <stdint.h>
// #include "parser_automaton.h"
#include "logger.h"
#include "pop3_parser_automaton.h"
#include <stdlib.h>
#include <sys/wait.h>
// #include "server_utils.h"
// #include "parserADT.h"

typedef struct Connection *pconn;

#define BASE_DIR "/tmp"
#define MAX_FILE_PATH 700
#define MAX_COMMAND_LENGTH 255  // octets
#define MAX_RESPONSE_LENGTH 512 // octets
#define BUFFER_SIZE 1024
#define MAX_BYTES 32768 // 2 to the power of 15

int parse_input(const uint8_t *input, pconn conn);

#endif
