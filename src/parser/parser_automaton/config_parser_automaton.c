#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config_parser_automaton.h"

// Definition of states
enum config_states
{
    INIT,
    TOKEN_1,
    TOKEN_2,
    TOKEN_3,
    TOKEN_4,
    TOKEN_5,
    TOKEN_6,
    TOKEN_7,
    TOKEN_8,
    TOKEN_9,
    TOKEN_10,
    OPERATION_0,
    OPERATION_1,
    OPERATION_2,
    OPERATION_3,
    OBJECT_CODE_0,
    OBJECT_CODE_1,
    OBJECT_CODE_2,
    OBJECT_CODE_3,
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

// Parser getter functions
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

    // Find the position of the newline character
    size_t newline_pos = strcspn(d->argument, "\n");

    // Copy the substring without the newline character
    strncpy(buff, d->argument, newline_pos);

    // Null-terminate the copied string
    buff[newline_pos] = '\0';
}

// Parser action functions
static parser_state token_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->token_length < 10)
    {
        d->token[d->token_length++] = c;
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state operation_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->operation_length < 3)
    {
        d->operation[d->operation_length++] = c;
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state object_code_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->object_code_length < 3)
    {
        d->object_code[d->object_code_length++] = c; // Convert to uppercase
        return PARSER_READING;
    }
    return PARSER_ERROR;
}

static parser_state argument_action(void *data, uint8_t c)
{
    config_parser_data *d = (config_parser_data *)data;
    if (d->argument_length < sizeof(d->argument) - 1)
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

static parser_state err_action(void *data, uint8_t c)
{
    return PARSER_ERROR;
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

static const struct parser_state_transition ST_INIT[] = {
    {.when = is_alphanumeric, .dest = TOKEN_1, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_1[] = {
    {.when = is_alphanumeric, .dest = TOKEN_2, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_2[] = {
    {.when = is_alphanumeric, .dest = TOKEN_3, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_3[] = {
    {.when = is_alphanumeric, .dest = TOKEN_4, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_4[] = {
    {.when = is_alphanumeric, .dest = TOKEN_5, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_5[] = {
    {.when = is_alphanumeric, .dest = TOKEN_6, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_6[] = {
    {.when = is_alphanumeric, .dest = TOKEN_7, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_7[] = {
    {.when = is_alphanumeric, .dest = TOKEN_8, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_8[] = {
    {.when = is_alphanumeric, .dest = TOKEN_9, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_9[] = {
    {.when = is_alphanumeric, .dest = TOKEN_10, .action = token_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_TOKEN_10[] = {
    {.when = is_space, .dest = OPERATION_0, .action = def_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OPERATION_0[] = {
    {.when = is_letter, .dest = OPERATION_1, .action = operation_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OPERATION_1[] = {
    {.when = is_letter, .dest = OPERATION_2, .action = operation_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OPERATION_2[] = {
    {.when = is_letter, .dest = OPERATION_3, .action = operation_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OPERATION_3[] = {
    {.when = is_space, .dest = OBJECT_CODE_0, .action = def_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OBJECT_CODE_0[] = {
    {.when = is_letter, .dest = OBJECT_CODE_1, .action = object_code_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OBJECT_CODE_1[] = {
    {.when = is_letter, .dest = OBJECT_CODE_2, .action = object_code_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OBJECT_CODE_2[] = {
    {.when = is_letter, .dest = OBJECT_CODE_3, .action = object_code_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_OBJECT_CODE_3[] = {
    {.when = is_space, .dest = ARGUMENT, .action = def_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_MAYEND[] = {
    {.when = is_lf, .dest = ENDST, .action = err_action},
    {.when = is_any, .dest = ERRST, .action = end_action},
};
static const struct parser_state_transition ST_ARGUMENT[] = {
    {.when = is_printable, .dest = ARGUMENT, .action = argument_action},
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};
static const struct parser_state_transition ST_END[] = {
    {.when = is_any, .dest = ENDST, .action = def_action},
};
static const struct parser_state_transition ST_ERR[] = {
    {.when = is_lf, .dest = MAYEND, .action = end_action},
    {.when = is_any, .dest = ERRST, .action = err_action},
};

static const struct parser_state_transition *config_states[] = {
    ST_INIT,
    ST_TOKEN_1,
    ST_TOKEN_2,
    ST_TOKEN_3,
    ST_TOKEN_4,
    ST_TOKEN_5,
    ST_TOKEN_6,
    ST_TOKEN_7,
    ST_TOKEN_8,
    ST_TOKEN_9,
    ST_TOKEN_10,
    ST_OPERATION_0,
    ST_OPERATION_1,
    ST_OPERATION_2,
    ST_OPERATION_3,
    ST_OBJECT_CODE_0,
    ST_OBJECT_CODE_1,
    ST_OBJECT_CODE_2,
    ST_OBJECT_CODE_3,
    ST_ARGUMENT,
    ST_MAYEND,
    ST_END,
    ST_ERR,
};

static const size_t config_states_n[] = {
    N(ST_INIT),
    N(ST_TOKEN_1),
    N(ST_TOKEN_2),
    N(ST_TOKEN_3),
    N(ST_TOKEN_4),
    N(ST_TOKEN_5),
    N(ST_TOKEN_6),
    N(ST_TOKEN_7),
    N(ST_TOKEN_8),
    N(ST_TOKEN_9),
    N(ST_TOKEN_10),
    N(ST_OPERATION_0),
    N(ST_OPERATION_1),
    N(ST_OPERATION_2),
    N(ST_OPERATION_3),
    N(ST_OBJECT_CODE_0),
    N(ST_OBJECT_CODE_1),
    N(ST_OBJECT_CODE_2),
    N(ST_OBJECT_CODE_3),
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
    .start_state = INIT,
    .init = config_parser_init,
    .copy = config_parser_copy,
    .reset = config_parser_reset,
    .destroy = config_parser_destroy,
};
