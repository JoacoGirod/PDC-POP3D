#ifndef __PARSER_ADT_H__
#define __PARSER_ADT_H__

#include <stdint.h>
#include "parser_automaton.h"
#include "logger.h"
#include "pop3_parser_automaton.h"
#include "server_utils.h"

typedef struct parserCDT
{
    const struct parser_automaton *def; // Automata
    uint8_t state;                      // Current state
    parser_state parser_state;          // Current parser state
    void *data;                         // Parser data
} parserCDT;

typedef struct parserCDT *parserADT;

parserADT parser_init(const parser_automaton *def);

void parser_destroy(parserADT p);

void parser_reset(parserADT p);

parser_state parser_feed(parserADT p, uint8_t c);

void *parser_get_data(parserADT p);

// Forward declaration for struct connection
struct connection;

int parse_input(const uint8_t *input, const struct connection *conn, struct buffer *dataSendingBuffer);

#endif
