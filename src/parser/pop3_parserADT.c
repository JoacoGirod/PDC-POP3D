#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "parserADT.h"
#include "pop3_functions.h"

#define CAPA_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define CAPA_AUTHORIZED_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define BASE_DIR "/tmp/Maildir"
#define MAX_FILE_PATH 700
#define MAX_COMMAND_LENGTH 255
#define BUFFER_SIZE 1024

// aux function for STAT command action
size_t calculate_total_email_bytes(struct Connection *conn)
{
    size_t totalBytes = 0;

    for (size_t i = 0; i < conn->num_emails; ++i)
    {
        totalBytes += conn->mails[i].octets;
    }

    return totalBytes;
}

// --------------------------------------------- ACTIONS ---------------------------------------------
int user_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    // microTesting();
    // retrieve_emails(BASE_DIR, conn);

    strcpy(conn->username, argument);
    // always sends OK
    send_data("+OK \r\n", dataSendingBuffer, conn);
    return 0;
}
int pass_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    struct GlobalConfiguration *gConf = get_global_configuration();

    // find user and see if passwords match
    for (size_t i = 0; i < get_global_configuration()->numUsers; i++)
    {
        // cycle through usernames and see if they match
        if (strcmp(conn->username, get_global_configuration()->users[i].name) == 0)
        {
            // user found, check if passwords match
            if (strcmp(get_global_configuration()->users[i].pass, argument) == 0)
            {
                // send OK
                send_data("+OK Logged in. \r\n", dataSendingBuffer, conn);

                // set connection status as TRANSACTION (user already authorized)
                conn->status = TRANSACTION;

                // retrieve user's emails
                char filePath[MAX_FILE_PATH];
                snprintf(filePath, sizeof(filePath), "%s/%s", gConf->maildir_folder, conn->username);
                retrieve_emails(filePath, conn);
                return 0;
            }
            else
            {
                // incorrect password
                send_data("-ERR [AUTH] Authentication failed. \r\n", dataSendingBuffer, conn);
                return 1;
            }
        }
    }
    // user not found
    send_data("-ERR [AUTH] Authentication failed. \r\n", dataSendingBuffer, conn);

    return 0;
}

int stat_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    if (conn->status == AUTHORIZATION)
    {
        send_data("-ERR Unkown command. \r\n", dataSendingBuffer, conn);
        return -1;
    }
    // calculates number of emails and total email bytes
    size_t numEmails = conn->num_emails;
    size_t totalEmailBytes = calculate_total_email_bytes(conn);

    // create response string
    char response[256];
    snprintf(response, sizeof(response), "+OK %zu %zu\r\n", numEmails, totalEmailBytes);

    // print the response string
    send_data(response, dataSendingBuffer, conn);

    return 0;
}

int list_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    if (conn->status == AUTHORIZATION)
    {
        send_data("-ERR Unkown command. \r\n", dataSendingBuffer, conn);
        return -1;
    }

    size_t numEmails = conn->num_emails;

    // creates the response header
    char responseHeader[64];
    snprintf(responseHeader, sizeof(responseHeader), "+OK %zu messages:\r\n", numEmails);

    // sends response header
    send_data(responseHeader, dataSendingBuffer, conn);

    // loops through each email and sends its index and size
    for (size_t i = 0; i < numEmails; ++i)
    {
        char emailInfo[64];
        snprintf(emailInfo, sizeof(emailInfo), "%zu %zu\r\n", i + 1, conn->mails[i].octets);
        send_data(emailInfo, dataSendingBuffer, conn);
    }

    // sends termination line
    send_data(".\r\n", dataSendingBuffer, conn);

    return 0;
}

int retr_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    if (conn->status == AUTHORIZATION)
    {
        send_data("-ERR Unkown command. \r\n", dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - Unknown command");
        return -1;
    }

    log_message(logger, INFO, COMMAND_HANDLER, " - Retr action started");

    struct GlobalConfiguration *gConf = get_global_configuration();

    // gets the email index and subtracts 1 because mails are numbered
    size_t mailIndex = atoi(argument) - 1;
    // checks if the emailIndex is valid
    if (mailIndex > conn->num_emails)
    {
        // invalid email index
        send_data("-ERR Invalid email index\r\n", dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - User entered invalid email index");
        return 1;
    }

    struct Mail *mail = &conn->mails[mailIndex];

    // constructs file path
    char filePath[MAX_FILE_PATH];
    snprintf(filePath, sizeof(filePath), "%s/%s/%s/%s", gConf->maildir_folder, conn->username, mail->folder, mail->filename);

    // Open the file for reading
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        log_message(logger, ERROR, COMMAND_HANDLER, " - Error opening mail file");
        return -1;
    }

    // sends initial response
    char initial[64];
    sprintf(initial, "+OK %zu octets\r\n", mail->octets);
    send_data(initial, dataSendingBuffer, conn);

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    log_message(logger, INFO, COMMAND_HANDLER, " - Printing mail data to client");
    while ((bytesRead = fread(buffer, 1, sizeof(buffer) - 1, file)) > 0)
    {
        buffer[bytesRead] = '\0';
        send_data(buffer, dataSendingBuffer, conn);
        memset(buffer, 0, sizeof(buffer));
    }

    log_message(logger, INFO, COMMAND_HANDLER, " - Mail data printed to client");

    // closes file
    fclose(file);

    // marks mail as RETRIEVED
    mail->status = RETRIEVED;
    log_message(logger, INFO, COMMAND_HANDLER, " - Mail marked as RETRIEVED");

    send_data(".\r\n", dataSendingBuffer, conn);

    return 0;
}

// sets specific Mail to state DELETED
int dele_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - Dele action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data("-ERR Unkown command. \r\n", dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - DELE command in AUTHORIZATION state forbidden");
        return -1;
    }

    size_t mailIndex = atoi(argument) - 1;

    // checks if the emailIndex is valid
    if (mailIndex > conn->num_emails)
    {
        // invalid email index
        send_data("-ERR Invalid email index\r\n", dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - User entered invalid email index");
        return -1;
    }

    // sets the mail status to DELETED
    conn->mails[mailIndex].status = DELETED;
    log_message(logger, INFO, COMMAND_HANDLER, " - Mail marked as DELETED");

    send_data("+OK Marked to be deleted. \r\n", dataSendingBuffer, conn);

    return 0;
}

int noop_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    if (conn->status == AUTHORIZATION)
    {
        send_data("-ERR Unkown command. \r\n", dataSendingBuffer, conn);
        return -1;
    }

    // simply responds +OK
    send_data("+OK\r\n", dataSendingBuffer, conn);

    return 0;
}

int rset_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    if (conn->status == AUTHORIZATION)
    {
        send_data("-ERR Unkown command. \r\n", dataSendingBuffer, conn);
        return -1;
    }

    // loops through each email and sets its status to UNCHANGED
    for (size_t i = 0; i < conn->num_emails; ++i)
    {
        conn->mails[i].status = UNCHANGED;
    }
    send_data("+OK \r\n", dataSendingBuffer, conn);

    return 0;
}

int quit_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMANDPARSER, " - Quit action started");

    // Envía la respuesta al cliente
    send_data("+OK POP3 server signing off. \r\n", dataSendingBuffer, conn);

    log_message(logger, INFO, CONNECTION, " - Closing connection");

    // Cierra la conexión si está en estado de AUTORIZACIÓN
    if (conn->status == AUTHORIZATION)
    {
        if (close(conn->fd) == -1)
        {
            log_message(logger, ERROR, CONNECTION, " - Error closing connection (AUTHORIZATION state)");
            return -1;
        }
        log_message(logger, INFO, CONNECTION, " - Connection closed (AUTHORIZATION state)");
        return 1;
    }

    // Marca la conexión como en estado de ACTUALIZACIÓN
    conn->status = UPDATE;
    log_message(logger, INFO, CONNECTION, " - Connection transitioned to UPDATE state");

    // Procesa cada correo electrónico
    for (size_t i = 0; i < conn->num_emails; ++i)
    {
        if (conn->mails[i].status == DELETED)
        {
            char filePath[MAX_FILE_PATH];
            snprintf(filePath, sizeof(filePath), "%s/%s/%s/%s", BASE_DIR, conn->username, conn->mails[i].folder, conn->mails[i].filename);
            delete_file(filePath);
        }
        else if (conn->mails[i].status == RETRIEVED)
        {
            if (move_file_new_to_cur(BASE_DIR, conn->username, conn->mails[i].filename) == -1)
            {
                log_message(logger, ERROR, CONNECTION, " - Error moving file from new to cur");
                return -1;
            }
        }
        // Restablece el estado del correo a UNCHANGED
        conn->mails[i].status = UNCHANGED;
    }
    log_message(logger, INFO, CONNECTION, " - All emails processed");

    // Cierra la conexión
    if (close(conn->fd) == -1)
    {
        log_message(logger, ERROR, CONNECTION, " - Error closing connection (UPDATE state)");
        return -1;
    }
    log_message(logger, INFO, CONNECTION, " - Connection closed (UPDATE state)");

    return 1;
}

int capa_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    if (conn->status == AUTHORIZATION)
    {
        send_data(CAPA_AUTHORIZED_MESSAGE, dataSendingBuffer, conn);
    }

    send_data(CAPA_MESSAGE, dataSendingBuffer, conn);

    return 0;
}

int default_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    // invalid command ya es tirado por parser, no se que hace esto
    send_data("-ERR Default Action \r\n", dataSendingBuffer, conn);

    return 0;
}

typedef int (*command_action)(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument);
// --------------------------------------------- COMMAND HANDLE ---------------------------------------------

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

int parse_input(const uint8_t *input, struct Connection *conn, struct buffer *dataSendingBuffer)
{
    // struct GlobalConfiguration *gConf = get_global_configuration();
    // Users array
    // gConf->users
    // Quantity of Users for cycling
    // gConf->numUsers

    // Logger conn->logger

    // send_data("Esto le reponde al usuario", dataSendingBuffer, conn);

    log_message(conn->logger, INFO, COMMANDPARSER, " - Parsing input: %s", input);
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
            log_message(conn->logger, INFO, COMMANDPARSER, " - Parser finished");
            break;
        }
        else if (result == PARSER_ERROR)
        {
            log_message(conn->logger, ERROR, COMMANDPARSER, " - Parser error");
            break;
        }
    }

    char cmd_buffer[CMD_LENGTH + 1];
    char arg_buffer[ARG_MAX_LENGTH + 1];
    get_pop3_cmd(pop3_parser, cmd_buffer, CMD_LENGTH + 1);
    get_pop3_arg(pop3_parser, arg_buffer, ARG_MAX_LENGTH + 1);

    pop3_command command = get_command(cmd_buffer);
    if (command == ERROR_COMMAND)
    {
        send_data("-ERR Unknown command\r\n", dataSendingBuffer, conn);
        log_message(conn->logger, ERROR, COMMANDPARSER, " - Unknown command");
    }
    if (!commands[command].accept_arguments(arg_buffer))
    {
        send_data("-ERR Bad argument\r\n", dataSendingBuffer, conn);
        log_message(conn->logger, ERROR, COMMANDPARSER, " - Bad argument");
    }
    else
    {
        int commandRet = commands[command].action(conn, dataSendingBuffer, arg_buffer);
        if (commandRet == -1)
        {
            log_message(conn->logger, ERROR, COMMANDPARSER, " - Error executing command");
        }
    }

    parser_destroy(pop3_parser);
    log_message(conn->logger, INFO, COMMANDPARSER, " - Parser destroyed");

    return 0;
}
