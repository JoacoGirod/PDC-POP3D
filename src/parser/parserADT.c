#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "parserADT.h"
typedef struct parserCDT *parserADT;

parserADT parser_init(const parser_automaton *def)
{
    parserADT p = calloc(1, sizeof(parserCDT));
    if (p == NULL || errno == ENOMEM)
    {
        printf("Error creating parserADT");
        return NULL;
    }
    if (def->init != NULL)
    {
        p->data = def->init();
        if (p->data == NULL)
        {
            printf("Error initializing parser data structure");
            free(p);
            return NULL;
        }
    }
    p->def = def;
    p->state = def->start_state;
    p->parser_state = PARSER_READING;
    return p;
}

void parser_destroy(parserADT p)
{
    if (p != NULL)
    {
        if (p->def->destroy != NULL)
        {
            p->def->destroy(p->data);
        }
        free(p);
    }
}

void parser_reset(parserADT p)
{
    p->state = p->def->start_state;
    p->parser_state = PARSER_READING;
    if (p->def->reset != NULL)
    {
        p->def->reset(p->data);
    }
}

void *parser_get_data(parserADT p)
{
    if (p->def->copy == NULL)
    {
        return NULL;
    }
    return p->def->copy(p->data);
}

parser_state parser_feed(parserADT p, uint8_t c)
{
    // Se obtienen las transiciones del estado actual
    const struct parser_state_transition *state = p->def->states[p->state];
    // Se obtiene la cantidad de transiciones del estado actual
    const size_t n = p->def->states_n[p->state];
    // Flag para indicar si se encontró una transición que con la condición
    bool matched = false;
    for (size_t i = 0; i < n && !matched; i++)
    {
        matched = state[i].when(c);
        if (matched)
        {
            p->state = state[i].dest;
            parser_state resp = state[i].action(p->data, c);
            if (resp == PARSER_FINISHED)
            {
                return (p->parser_state == PARSER_ERROR) ? p->parser_state : resp;
            }
            else if (resp == PARSER_ERROR)
            {
                p->parser_state = PARSER_ERROR;
            }
            else if (resp == PARSER_ACTION)
            {
                return resp;
            }
        }
    }
    return PARSER_READING;
}

bool has_argument(const char *arg)
{
    return strlen(arg) != 0;
}

bool can_have_argument(const char *arg)
{
    return true;
}

bool hasnt_argument(const char *arg)
{
    return arg == NULL || strlen(arg) == 0;
}

int user_action(const char *arg)
{
    printf("user_action\n");
    return 0;
}
int pass_action(const char *arg)
{
    printf("pass_action\n");
    return 0;
}
int stat_action(const char *arg)
{
    printf("stat_action\n");
    return 0;
}
int list_action(const char *arg)
{
    printf("list_action\n");
    return 0;
}
int retr_action(const char *arg)
{
    printf("retr_action\n");
    return 0;
}
int dele_action(const char *arg)
{
    printf("dele_action\n");
    return 0;
}
int noop_action(const char *arg)
{
    printf("noop_action\n");
    return 0;
}
int rset_action(const char *arg)
{
    printf("rset_action\n");
    return 0;
}
int quit_action(const char *arg)
{
    printf("quit_action\n");
    return 0;
}
int capa_action(const char *arg)
{
    printf("capa_action\n");
    return 0;
}
int default_action(const char *arg)
{
    printf("default_action\n");
    return 0;
}

typedef int (*command_action)(const char *arg);

struct command
{
    const char *name;                          // command name
    bool (*accept_arguments)(const char *arg); // function to accept arguments or not
    command_action action;                     // action to execute
};
static struct command commands[] = {
    {.name = "USER",
     .accept_arguments = has_argument,
     .action = user_action},
    {.name = "PASS",
     .accept_arguments = has_argument,
     .action = pass_action},
    {.name = "STAT",
     .accept_arguments = hasnt_argument,
     .action = stat_action},
    {.name = "LIST",
     .accept_arguments = can_have_argument,
     .action = list_action},
    {.name = "RETR",
     .accept_arguments = has_argument,
     .action = retr_action},
    {.name = "DELE",
     .accept_arguments = has_argument,
     .action = dele_action},
    {.name = "NOOP",
     .accept_arguments = hasnt_argument,
     .action = noop_action},
    {.name = "RSET",
     .accept_arguments = hasnt_argument,
     .action = rset_action},
    {.name = "QUIT",
     .accept_arguments = hasnt_argument,
     .action = quit_action},
    {
        .name = "CAPA",
        .accept_arguments = hasnt_argument,
        .action = capa_action,
    },
    {.name = "Error command",
     .accept_arguments = can_have_argument,
     .action = default_action}};

typedef enum
{
    USER = 0,
    PASS,
    STAT,
    LIST,
    RETR,
    DELE,
    NOOP,
    RSET,
    QUIT,
    CAPA,
    ERROR_COMMAND
} pop3_command;

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;
    while (n-- != 0 && tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
    {
        if (n == 0 || *s1 == '\0' || *s2 == '\0')
            break;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

pop3_command get_command(const char *command)
{
    for (int i = USER; i <= CAPA; i++)
    {
        if (strncasecmp(command, commands[i].name, 4) == 0)
        {
            return i;
        }
    }
    return ERROR_COMMAND;
}
size_t u_strlen(const uint8_t *s)
{
    size_t i = 0;
    while (s[i] != '\0')
    {
        i++;
    }
    return i;
}

int parse_input(const uint8_t *input, const struct connection *conn, struct buffer *dataSendingBuffer)
{
    // struct GlobalConfiguration *gConf = get_global_configuration();
    // Users array
    // gConf->users
    // Quantity of Users for cycling
    // gConf->numUsers

    // Logger conn->logger

    // send_data("Esto le reponde al usuario", dataSendingBuffer, conn);

    log_message(conn->logger, INFO, COMMANDPARSER, "Parsing input: %s", input);
    extern const parser_automaton pop3_parser_automaton;

    parserADT pop3_parser = parser_init(&pop3_parser_automaton);

    // Input string to simulate parsing
    size_t input_length = u_strlen(input);

    // Feed each character to the parser
    for (size_t i = 0; i < input_length; i++)
    {
        parser_state result = parser_feed(pop3_parser, input[i]);

        if (result == PARSER_FINISHED)
        {
            printf("Parser finished successfully.\n");
            break;
        }
        else if (result == PARSER_ERROR)
        {
            printf("Parser encountered an error.\n");
            break;
        }
    }

    // Get and print the parsed data
    char cmd_buffer[CMD_LENGTH + 1];
    char arg_buffer[ARG_MAX_LENGTH + 1];
    get_pop3_cmd(pop3_parser, cmd_buffer, CMD_LENGTH + 1);
    get_pop3_arg(pop3_parser, arg_buffer, ARG_MAX_LENGTH + 1);

    pop3_command command = get_command(cmd_buffer);
    if (command == ERROR_COMMAND)
    {
        printf("Invalid command\n");
    }
    if (!commands[command].accept_arguments(arg_buffer))
    {
        printf("Bad argument\n");
    }
    else
    {
        printf("Command: %s\n", commands[command].name);
        int commandRet = commands[command].action("TODO ACTION");
        if (commandRet == -1)
        {
            printf("Error executing command\n");
        }
    }

    parser_destroy(pop3_parser);

    return 0;
}
