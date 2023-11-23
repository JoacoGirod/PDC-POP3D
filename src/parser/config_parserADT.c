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
#define SUC_BUFFER_SIZE "+SUC Buffer size: "
#define SUC_LOGS_FOLDER "+SUC Logs folder: "
#define SUC_MAILDIR_FOLDER "+SUC Maildir folder: "
#define SUC_AUTH_TOKEN "+SUC Authorization token: "
#define SUC_NEW_BUFF_SIZE "+SUC New buffer size: "
#define SUC_NEW_LOGS_FOLDER "+SUC New logs folder: "
#define SUC_NEW_MAILDIR_FOLDER "+SUC New maildir folder: "
#define SUC_NEW_AUTH_TOKEN "+SUC New authorization token: "
#define SUC_HISTORIC_TOTAL_USERS "+SUC Historic total users: "
#define SUC_CONCURRENT_USERS "+SUC Concurrent users: "
#define SUC_BYTES_TRANSFERRED "+SUC Bytes transferred: "
#define SUC_NEW_TRANSFORMATION_TRUE "+SUC Transformation set: true\n"
#define SUC_NEW_TRANSFORMATION_FALSE "+SUC Transformation set: false\n"
#define SUC_TRANSFORMATION_TRUE "+SUC Transformation active: true\n"
#define SUC_TRANSFORMATION_FALSE "+SUC Transformation active: false\n"

#define ERR_INVALID_AUTH_TOKEN "-ERR Invalid authorization token\n"
#define ERR_AUTH_TOKEN_LENGHT "-ERR New authorization token must be 10 characters long\n"
#define ERR_EXECUTING_COMMAND "-ERR Error executing command\n"
#define ERR_INVALID_OBJECT_CODE "-ERR Invalid object code\n"
#define ERR_SECOND_COMMAND_GET_SET "-ERR Second command must be GET or SET\n"
#define ERR_ARGUMENT_EXPECTED_SET "-ERR Argument expected for SET command\n"
#define ERR_NO_ARGUMENT_EXPECTED_GET "-ERR No argument expected for GET command\n"

// --------------------------------------------- ACTIONS ---------------------------------------------
int getbuf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    char buffer_size[32];
    sprintf(buffer_size, "%d", (int)g_conf->buffers_size);
    send_data_udp(logger, client_info, SUC_BUFFER_SIZE);
    send_data_udp(logger, client_info, buffer_size);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int getlgf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    send_data_udp(logger, client_info, SUC_LOGS_FOLDER);
    send_data_udp(logger, client_info, g_conf->logs_folder);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int getmdf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    send_data_udp(logger, client_info, SUC_MAILDIR_FOLDER);
    send_data_udp(logger, client_info, g_conf->maildir_folder);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int getatt_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    send_data_udp(logger, client_info, SUC_AUTH_TOKEN);
    send_data_udp(logger, client_info, g_conf->authorization_token);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int setbuf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    g_conf->buffers_size = atoi(argument);
    send_data_udp(logger, client_info, SUC_NEW_BUFF_SIZE);
    send_data_udp(logger, client_info, argument);
    send_data_udp(logger, client_info, "\n");
    return 0;
}

int setlgf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    strcpy(g_conf->logs_folder, argument);
    send_data_udp(logger, client_info, SUC_NEW_LOGS_FOLDER);
    send_data_udp(logger, client_info, g_conf->logs_folder);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int setmdf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    strcpy(g_conf->maildir_folder, argument);
    g_conf->maildir_folder[strlen(argument)] = '\0';
    send_data_udp(logger, client_info, SUC_NEW_MAILDIR_FOLDER);
    send_data_udp(logger, client_info, g_conf->maildir_folder);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int setatt_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    if (strlen(argument) - 1 != 10)
    {
        send_data_udp(logger, client_info, ERR_AUTH_TOKEN_LENGHT);
    }
    else
    {
        strcpy(g_conf->authorization_token, argument);
        send_data_udp(logger, client_info, SUC_NEW_AUTH_TOKEN);
        send_data_udp(logger, client_info, g_conf->authorization_token);
        send_data_udp(logger, client_info, "\n");
    }
    return 0;
}
int gethtu_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    char htu_buffer[32];
    sprintf(htu_buffer, "%d", (int)g_stats->total_clients);
    send_data_udp(logger, client_info, SUC_HISTORIC_TOTAL_USERS);
    send_data_udp(logger, client_info, htu_buffer);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int getccu_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    char ccu_buffer[32];
    sprintf(ccu_buffer, "%d", (int)g_stats->concurrent_clients);
    send_data_udp(logger, client_info, SUC_CONCURRENT_USERS);
    send_data_udp(logger, client_info, ccu_buffer);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int getbtf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    char btf_buffer[32];
    sprintf(btf_buffer, "%d", (int)g_stats->concurrent_clients);
    send_data_udp(logger, client_info, SUC_BYTES_TRANSFERRED);
    send_data_udp(logger, client_info, btf_buffer);
    send_data_udp(logger, client_info, "\n");
    return 0;
}
int settrf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    if (strcmp(argument, "true") == 0)
    {
        g_conf->transformation = true;
        send_data_udp(logger, client_info, SUC_NEW_TRANSFORMATION_TRUE);
    }
    else if (strcmp(argument, "false") == 0)
    {
        g_conf->transformation = false;
        send_data_udp(logger, client_info, SUC_NEW_TRANSFORMATION_FALSE);
    }
    else
    {
        send_data_udp(logger, client_info, SUC_NEW_TRANSFORMATION_FALSE);
        return -1;
    }

    return 0;
}
int gettrf_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    if (g_conf->transformation)
    {
        send_data_udp(logger, client_info, SUC_TRANSFORMATION_TRUE);
    }
    else
    {
        send_data_udp(logger, client_info, SUC_TRANSFORMATION_FALSE);
    }
    return 0;
}
int config_default_action(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats)
{
    send_data_udp(logger, client_info, "Invalid command\n");
    return 0;
}

typedef int (*command_action)(char *argument, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf, struct GlobalStatistics *g_stats);
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
    {.name = "GETTRF",
     .accept_arguments = hasnt_argument,
     .action = gettrf_action},
    {.name = "SETTRF",
     .accept_arguments = has_argument,
     .action = settrf_action},
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
    GETTRF,
    SETTRF,
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
    BTF,
    TRF
} config_object_code;

typedef enum
{
    GET = 0,
    SET,
    ERROR_OPERATION
} config_operation;

static bool is_valid_token(const char *token, Logger *logger, const pUDPClientInfo client_info, struct GlobalConfiguration *g_conf)
{
    // Handle authorization token
    if (strncmp(g_conf->authorization_token, token, 10) != 0)
    {
        // Authorization token is invalid
        send_data_udp(logger, client_info, ERR_INVALID_AUTH_TOKEN);
        return false;
    }
    return true;
}

static bool is_valid_get_set(const char *operation, const char *argument, Logger *logger, const pUDPClientInfo client_info)
{
    if (strncasecmp(operation, "GET", 3) == 0)
    {
        if (has_argument(argument))
        {
            send_data_udp(logger, client_info, ERR_INVALID_OBJECT_CODE);
            return false;
        }
        return true;
    }
    else if (strncasecmp(operation, "SET", 3) == 0)
    {
        if (!has_argument(argument))
        {
            send_data_udp(logger, client_info, ERR_ARGUMENT_EXPECTED_SET);
            return false;
        }
        return true;
    }
    else
    {
        send_data_udp(logger, client_info, ERR_SECOND_COMMAND_GET_SET);
        return false;
    }
}

static bool is_valid_object_code(const char *object_code, Logger *logger, const pUDPClientInfo client_info)
{
    for (int i = BUF; i <= TRF; i++)
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
        case TRF:
            enum_name = "TRF";
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
    send_data_udp(logger, client_info, ERR_INVALID_OBJECT_CODE);
    return false;
}

config_command get_config_command(const char *command)
{
    for (int i = GETBUF; i <= SETTRF; i++)
    {
        if (strncasecmp(command, config_commands[i].name, 6) == 0)
        {
            return i;
        }
    }
    return ERROR_COMMAND;
}

int config_parse_input(Logger *logger, const pUDPClientInfo client_info, uint8_t *input)
{
    extern const parser_automaton config_parser_automaton;
    parserADT config_parser = parser_init(&config_parser_automaton);

    // Input string to simulate parsing
    size_t input_length = u_strlen(input);

    // Feed each character to the parser
    for (size_t i = 0; i < input_length; i++)
    {
        parser_state result = parser_feed(config_parser, input[i]);

        if (result == PARSER_FINISHED)
        {
            log_message(logger, INFO, CONFIGPARSER, " - Parser finished");
            break;
        }
        else if (result == PARSER_ERROR)
        {
            log_message(logger, ERROR, CONFIGPARSER, " - Parser finished");
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

    struct GlobalConfiguration *g_conf = get_global_configuration();
    struct GlobalStatistics *g_stats = get_global_statistics();

    // validate auth token:
    if (is_valid_token(token_buffer, logger, client_info, g_conf) && is_valid_get_set(operation_buffer, arg_buffer, logger, client_info) && is_valid_object_code(object_code_buffer, logger, client_info))
    {
        // concatenate operation and object code
        char command_buffer[OPERATION_LENGTH + OBJECT_CODE_LENGTH + 1];
        strcpy(command_buffer, operation_buffer);
        strcat(command_buffer, object_code_buffer);

        config_command command = get_config_command(command_buffer);
        int operationRet = config_commands[command].action(arg_buffer, logger, client_info, g_conf, g_stats);
        if (operationRet == -1)
        {
            send_data_udp(logger, client_info, ERR_EXECUTING_COMMAND);
        }
    }

    parser_destroy(config_parser);

    return 0;
}
