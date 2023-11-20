#ifndef __CONFIG_PARSER_AUTOMATON_H__
#define __CONFIG_PARSER_AUTOMATON_H__

#include <stdint.h>
#include "parser_automaton.h"
#include <server_utils.h>

#define TOKEN_LENGTH 10
#define OPERATION_LENGTH 3
#define OBJECT_CODE_LENGTH 3
#define ARGUMENT_LENGTH 234
typedef struct parserCDT *parserADT;

// Definition of parser data structure
typedef struct
{
    char token[TOKEN_LENGTH + 1];
    char operation[OPERATION_LENGTH + 1];
    char object_code[OBJECT_CODE_LENGTH + 1];
    char argument[ARGUMENT_LENGTH];
    size_t token_length;
    size_t operation_length;
    size_t object_code_length;
    size_t argument_length;
} config_parser_data;

/**
 * Places in buff the content of the token
 */
void get_config_token(parserADT p, char *buff, int max);

/**
 * Places in buff the content of the operation
 */
void get_config_operation(parserADT p, char *buff, int max);

/**
 * Places in buff the content of the command
 */
void get_config_object_code(parserADT p, char *buff, int max);

/**
 * Places in buff the content of the argument
 */
void get_config_argument(parserADT p, char *buff, int max);

#endif
