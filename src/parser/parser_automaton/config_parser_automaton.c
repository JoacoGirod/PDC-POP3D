#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config_parser_automaton.h"

// Definition of states
enum config_states
{
    TOKEN,
    OPERATION,
    OBJECT_CODE,
    ARGUMENT,
    MAYEND,
    ENDST,
    ERRST
};

// Initialization:
static void *config_parser_copy(void *data);
static void *config_parser_init(void);
static void config_parser_reset(void *data);
static void config_parser_destroy(void *data);

// Parser condition functions

void get_config_token(parserADT p, char *buff, int max)
{
    config_parser_data *d = (config_parser_data *)p->data;
    strncpy(buff, d->token, max - 1);
    buff[max - 1] = '\0';
}

void get_config_operation(parserADT p, char *buff, int max)
{
    config_parser_data *d = (config_parser_data *)p->data;
    strncpy(buff, d->operation, max - 1);
    buff[max - 1] = '\0';
}

void get_config_object_code(parserADT p, char *buff, int max)
{
    config_parser_data *d = (config_parser_data *)p->data;
    strncpy(buff, d->object_code, max - 1);
    buff[max - 1] = '\0';
}

void get_config_argument(parserADT p, char *buff, int max)
{
    config_parser_data *d = (config_parser_data *)p->data;
    strncpy(buff, d->argument, max - 1);
    buff[max - 1] = '\0';
}

// Parser action functions
static parser_state token_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->token_length < 10 && is_alphanumeric(c))
    {
        d->token[d->token_length++] = c;
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state operation_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->operation_length < 3 && is_letter(c))
    {
        d->operation[d->operation_length++] = toupper(c);
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state object_code_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->object_code_length < 3 && is_letter(c))
    {
        d->object_code[d->object_code_length++] = toupper(c); // Convert to uppercase
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state argument_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->argument_length < sizeof(d->argument) - 1 && is_any(c))
    {
        d->argument[d->argument_length++] = c;
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state end_action(void *data, uint8_t c)
{
    return PARSER_FINISHED;
}

static parser_state def_action(void *data, uint8_t c)
{
    return PARSER_READING;
}

// Initialization function
static void *config_parser_init(void)
{
    config_parser_data *data = calloc(1, sizeof(config_parser_data));
    if (data == NULL)
    {
        return NULL;
    }
    return data;
}

// Copy function
static void *config_parser_copy(void *data)
{
    config_parser_data *d = (config_parser_data *)data;
    config_parser_data *copy = calloc(1, sizeof(config_parser_data));
    if (copy == NULL)
    {
        return NULL;
    }
    copy->token_length = d->token_length;
    copy->operation_length = d->operation_length;
    copy->object_code_length = d->object_code_length;
    copy->argument_length = d->argument_length;
    strncpy(copy->token, d->token, d->token_length);
    strncpy(copy->operation, d->operation, d->operation_length);
    strncpy(copy->object_code, d->object_code, d->object_code_length);
    strncpy(copy->argument, d->argument, d->argument_length);
    return copy;
}

// Reset function
static void config_parser_reset(void *data)
{
    config_parser_data *d = (config_parser_data *)data;
    d->token_length = 0;
    d->operation_length = 0;
    d->object_code_length = 0;
    d->argument_length = 0;
    memset(d->token, 0, sizeof(d->token));
    memset(d->operation, 0, sizeof(d->operation));
    memset(d->object_code, 0, sizeof(d->object_code));
    memset(d->argument, 0, sizeof(d->argument));
}

// Destruction function
static void config_parser_destroy(void *data)
{
    free(data);
}

static const struct parser_state_transition ST_TOKEN[] = {
    {.when = is_alphanumeric, .dest = TOKEN, .action = token_action},
    {.when = is_space, .dest = OPERATION, .action = def_action},
    {.when = is_any, .dest = ERRST, .action = def_action},
};

static const struct parser_state_transition ST_OPERATION[] = {
    {.when = is_letter, .dest = OPERATION, .action = operation_action},
    {.when = is_space, .dest = OBJECT_CODE, .action = def_action},
    {.when = is_any, .dest = ERRST, .action = def_action},
};

static const struct parser_state_transition ST_OBJECT_CODE[] = {
    {.when = is_letter, .dest = OBJECT_CODE, .action = object_code_action},
    {.when = is_space, .dest = ARGUMENT, .action = def_action},
    {.when = is_any, .dest = ERRST, .action = def_action},
};

static const struct parser_state_transition ST_ARGUMENT[] = {
    {.when = is_any, .dest = ARGUMENT, .action = argument_action},
};

static const struct parser_state_transition ST_MAYEND[] = {
    {.when = is_space, .dest = MAYEND, .action = def_action},
    {.when = is_any, .dest = ERRST, .action = def_action},
};

static const struct parser_state_transition ST_END[] = {
    {.when = is_any, .dest = ENDST, .action = end_action},
};

static const struct parser_state_transition ST_ERR[] = {
    {.when = is_any, .dest = ERRST, .action = def_action},
};

static const struct parser_state_transition *config_states[] = {
    ST_TOKEN,
    ST_OPERATION,
    ST_OBJECT_CODE,
    ST_ARGUMENT,
    ST_MAYEND,
    ST_END,
    ST_ERR,
};

static const size_t config_states_n[] = {
    N(ST_TOKEN),
    N(ST_OPERATION),
    N(ST_OBJECT_CODE),
    N(ST_ARGUMENT),
    N(ST_MAYEND),
    N(ST_END),
    N(ST_ERR),
};

// Define the automaton structure
const parser_automaton config_parser_automaton = {
    .states_count = N(config_states),
    .states = config_states,
    .states_n = config_states_n,
    .start_state = TOKEN,
    .init = config_parser_init,
    .copy = config_parser_copy,
    .reset = config_parser_reset,
    .destroy = config_parser_destroy,
};
