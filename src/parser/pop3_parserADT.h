#ifndef __POP3_PARSER_ADT_H__
#define __POP3_PARSER_ADT_H__

#include <stdint.h>
// #include "parser_automaton.h"
#include "logger.h"
#include "pop3_parser_automaton.h"
// #include "server_utils.h"
// #include "parserADT.h"

typedef struct Connection *pconn;

int parse_input(const uint8_t *input, pconn conn, struct buffer *dataSendingBuffer);

#endif
