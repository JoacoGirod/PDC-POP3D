#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "config_parserADT.h"
#include "config_parser_automaton.h"

// Define the valid object codes for SET and GET
#define SET_AND_GET_OBJECT_CODES "BUF", "LGF", "MDF", "ATT"
#define GET_ONLY_OBJECT_CODES "HTU", "CCU", "BTF"

// --------------------------------------------- ACTIONS ---------------------------------------------
int getbuf_action(char *argument)
{

    return 0;
}
int getlgf_action(char *argument)
{
    return 0;
}
int getmdf_action(char *argument)
{

    return 0;
}
int getatt_action(char *argument)
{

    return 0;
}
int setbuf_action(char *argument)
{

    return 0;
}
int setlgf_action(char *argument)
{

    return 0;
}
int setmdf_action(char *argument)
{

    return 0;
}
int setatt_action(char *argument)
{

    return 0;
}
int gethtu_action(char *argument)
{
    return 0;
}
int getccu_action(char *argument)
{
    fprintf(stdout, "CCU action\n");

    return 0;
}
int getbtf_action(char *argument)
{
    fprintf(stdout, "BTF action\n");

    return 0;
}
int config_default_action(char *argument)
{
    fprintf(stdout, "Invalid command\n");

    return 0;
}

typedef int (*command_action)(char *argument);
// --------------------------------------------- COMMAND HANDLE ---------------------------------------------

struct command
{
    const char *name;                          // command name
    bool (*accept_arguments)(const char *arg); // function to accept arguments or not
    command_action action;                     // action to execute
};

struct command config_commands[] = {
    {.name = "GETBUF",
     .accept_arguments = hasnt_argument,
     .action = getbuf_action},
    {.name = "GETLGF",
     .accept_arguments = hasnt_argument,
     .action = getlgf_action},
    {.name = "GETMDF",
     .accept_arguments = hasnt_argument,
     .action = getmdf_action},
    {.name = "GETATT",
     .accept_arguments = hasnt_argument,
     .action = getatt_action},
    {.name = "SETBUF",
     .accept_arguments = has_argument,
     .action = setbuf_action},
    {.name = "SETLGF",
     .accept_arguments = has_argument,
     .action = setlgf_action},
    {.name = "SETMDF",
     .accept_arguments = has_argument,
     .action = setmdf_action},
    {.name = "SETATT",
     .accept_arguments = has_argument,
     .action = setatt_action},
    {.name = "GETHTU",
     .accept_arguments = hasnt_argument,
     .action = gethtu_action},
    {.name = "GETCCU",
     .accept_arguments = hasnt_argument,
     .action = getccu_action},
    {.name = "GETBTF",
     .accept_arguments = hasnt_argument,
     .action = getbtf_action},
    {.name = "Error command",
     .accept_arguments = can_have_argument,
     .action = config_default_action}};

typedef enum
{
    GETBUF = 0,
    GETLGF,
    GETMDF,
    GETATTATT,
    SETBUF,
    SETLGF,
    SETMDF,
    SETATT,
    GETHTU,
    GETCCU,
    GETBTF,
    ERROR_COMMAND
} config_command;

typedef enum
{
    BUF = 0,
    LGF,
    MDF,
    ATT,
    HTU,
    CCU,
    BTF
} config_object_code;

typedef enum
{
    GET = 0,
    SET,
    ERROR_OPERATION
} config_operation;

// Command actions
static bool is_valid_token(const char *token)
{
    // Handle authorization token
    if (true)
    {
        // Authorization token is valid
        return true;
    }
    else
    {
        // Authorization token is invalid
        // int send_data_udp(Logger *logger, const struct UDPClientInfo *client_info, uint8_t *data)
        // send_data_udp();
        fprintf(stdout, "Invalid authorization token\n");
        return false;
    }
}

static bool is_valid_get_set(const char *operation, const char *argument)
{
    if (strncasecmp(operation, "GET", 3) == 0)
    {
        if (has_argument(argument))
        {
            fprintf(stdout, "No Argument expected for GET command\n");
            return false;
        }
        return true;
    }
    else if (strncasecmp(operation, "SET", 3) == 0)
    {
        if (!has_argument(argument))
        {
            fprintf(stdout, "Argument expected for SET command\n");
            return false;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static bool is_valid_object_code(const char *object_code)
{
    for (int i = BUF; i <= BTF; i++)
    {
        const char *enum_name = NULL;

        switch (i)
        {
        case BUF:
            enum_name = "BUF";
            break;
        case LGF:
            enum_name = "LGF";
            break;
        case MDF:
            enum_name = "MDF";
            break;
        case ATT:
            enum_name = "ATT";
            break;
        case HTU:
            enum_name = "HTU";
            break;
        case CCU:
            enum_name = "CCU";
            break;
        case BTF:
            enum_name = "BTF";
            break;
        default:
            // Handle default case if needed
            break;
        }

        if (strncasecmp(object_code, enum_name, 3) == 0)
        {
            return true;
        }
    }

    return false;
}

config_command get_config_command(const char *command)
{
    for (int i = GETBUF; i <= GETBTF; i++)
    {
        if (strncasecmp(command, config_commands[i].name, 3) == 0)
        {
            return i;
        }
    }
    return ERROR_COMMAND;
}

int config_parse_input(Logger *logger, const pUDPClientInfo client_info, buffer *p_buffer, uint8_t *input)
{
    send_data_udp(logger, client_info, p_buffer, "FELIX: inside config parse input ------- ");
    send_data_udp(logger, client_info, p_buffer, (char *)input);
    send_data_udp(logger, client_info, p_buffer, " -------");
    extern const parser_automaton config_parser_automaton;
    parserADT config_parser = parser_init(&config_parser_automaton);

    // Input string to simulate parsing
    size_t input_length = u_strlen(input);

    // Feed each character to the parser
    for (size_t i = 0; i < input_length; i++)
    {
        send_data_udp(logger, client_info, p_buffer, "FELIX: ABOUT TO PARSER FEED\n");
        parser_state result = parser_feed(config_parser, input[i]);

        if (result == PARSER_FINISHED)
        {
            send_data_udp(logger, client_info, p_buffer, "FELIX: parser success\n");
            printf("Parser finished successfully.\n");
            break;
        }
        else if (result == PARSER_ERROR)
        {
            send_data_udp(logger, client_info, p_buffer, "FELIX: parser error\n");
            printf("Parser encountered an error.\n");
            break;
        }
    }

    char token_buffer[TOKEN_LENGTH + 1];
    char operation_buffer[OPERATION_LENGTH + 1];
    char object_code_buffer[OBJECT_CODE_LENGTH + 1];
    char arg_buffer[ARG_MAX_LENGTH + 1];

    get_config_token(config_parser, token_buffer, TOKEN_LENGTH + 1);
    get_config_operation(config_parser, operation_buffer, OPERATION_LENGTH + 1);
    get_config_object_code(config_parser, object_code_buffer, OBJECT_CODE_LENGTH + 1);
    get_config_argument(config_parser, arg_buffer, ARG_MAX_LENGTH + 1);

    send_data_udp(logger, client_info, p_buffer, "\ntoken: ");
    send_data_udp(logger, client_info, p_buffer, token_buffer);
    send_data_udp(logger, client_info, p_buffer, "\n operation: ");
    send_data_udp(logger, client_info, p_buffer, operation_buffer);
    send_data_udp(logger, client_info, p_buffer, "\n object code: ");
    send_data_udp(logger, client_info, p_buffer, object_code_buffer);
    send_data_udp(logger, client_info, p_buffer, "\n arg: ");
    send_data_udp(logger, client_info, p_buffer, arg_buffer);
    send_data_udp(logger, client_info, p_buffer, "\n");

    // validate auth token:
    if (is_valid_token(token_buffer) && is_valid_get_set(operation_buffer, arg_buffer) && is_valid_object_code(object_code_buffer))
    {
        send_data_udp(logger, client_info, p_buffer, "FELIX: validated token, get/set and OC\n");
        // concatenate operation and object code
        char command_buffer[OPERATION_LENGTH + OBJECT_CODE_LENGTH + 1];
        strcpy(command_buffer, operation_buffer);
        strcat(command_buffer, object_code_buffer);

        send_data_udp(logger, client_info, p_buffer, "FELIX: concat command: ");
        send_data_udp(logger, client_info, p_buffer, command_buffer);
        send_data_udp(logger, client_info, p_buffer, "\n\n");

        config_command command = get_config_command(command_buffer);
        int operationRet = config_commands[command].action(arg_buffer);
        if (operationRet == -1)
        {
            send_data_udp(logger, client_info, p_buffer, "Error executing command\n");
        }
    }

    parser_destroy(config_parser);

    return 0;
}
