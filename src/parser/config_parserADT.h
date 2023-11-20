#ifndef __CONFIG_PARSER_ADT_H__
#define __CONFIG_PARSER_ADT_H__

#include <stdint.h>
#include "parser_automaton.h"
#include <logger.h>
#include "config_parser_automaton.h"
#include <server_utils.h>
#include "parserADT.h"

int config_parse_input(const uint8_t *input, struct buffer *dataSendingBuffer);

#endif
