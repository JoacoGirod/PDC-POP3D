#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "parserADT.h"
#include "pop3_functions.h"

#define BASE_DIR "/tmp"
#define MAX_FILE_PATH 700
#define MAX_COMMAND_LENGTH 255  // octets
#define MAX_RESPONSE_LENGTH 512 // octets
#define BUFFER_SIZE 1024

#define OK_MARKED_DELETED "+OK Marked to be deleted. \r\n"
#define OK_SIGN_OFF "+OK POP3 server signing off.\r\n"
#define OK_RESPONSE "+OK\r\n"
#define OK_LOGGED_IN_RESPONSE "+OK Logged in.\r\n"
#define OK_CAPA_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define OK_CAPA_AUTHORIZED_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"

#define ERR_RESPONSE "-ERR\r\n"
#define ERR_AUTH_RESPONSE "-ERR [AUTH] Authentication failed.\r\n"
#define ERR_UNKNOWN_COMMAND_RESPONSE "-ERR Unknown command.\r\n"
#define ERR_BAD_ARGUMENT_RESPONSE "-ERR Bad argument.\r\n"
#define ERR_INVALID_EMAIL_INDEX_RESPONSE "-ERR Invalid email index.\r\n"
#define ERR_DEFAULT_ACTION "-ERR Command not found.\r\n"
#define ERR_NO_USERNAME_GIVEN_RESPONSE "-ERR No username given.\r\n"
#define DOT_RESPONSE ".\r\n"

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
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - USER: User action started");
    // microTesting();
    // retrieve_emails(BASE_DIR, conn);

    strcpy(conn->username, argument);
    // always sends OK
    send_data(OK_RESPONSE, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - USER: User action finished");
    return 0;
}
int pass_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Pass action started");

    if (strcmp(conn->username, "") == 0)
    {
        send_data(ERR_NO_USERNAME_GIVEN_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, INFO, COMMAND_HANDLER, " - PASS: No user given");
        return -1;
    }
    struct GlobalConfiguration *gConf = get_global_configuration();

    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Searching for user to authenticate");
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
                send_data(OK_LOGGED_IN_RESPONSE, dataSendingBuffer, conn);
                log_message(logger, INFO, COMMAND_HANDLER, " - PASS: User authenticated");
                // set connection status as TRANSACTION (user already authorized)
                conn->status = TRANSACTION;

                // retrieve user's emails
                char filePath[MAX_FILE_PATH];
                size_t filePathLength = strlen(gConf->mailroot_folder) + strlen(conn->username) + strlen(gConf->maildir_folder) + 3;
                snprintf(filePath, filePathLength, "%s/%s/%s", gConf->mailroot_folder, conn->username, gConf->maildir_folder);
                filePath[filePathLength] = '\0';
                retrieve_emails(filePath, conn);
                return 0;
            }
            else
            {
                // incorrect password
                memset(conn->username, '\0', sizeof(conn->username));
                send_data(ERR_AUTH_RESPONSE, dataSendingBuffer, conn);
                log_message(logger, ERROR, COMMAND_HANDLER, " - PASS: Incorrect password");
                return -1;
            }
        }
    }
    // user not found
    send_data(ERR_AUTH_RESPONSE, dataSendingBuffer, conn);
    log_message(logger, ERROR, COMMAND_HANDLER, " - PASS: User not found");
    return 0;
}

int stat_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - STAT: Stat action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - STAT: STAT command in AUTHORIZATION state forbidden");
        return -1;
    }
    // calculates number of emails and total email bytes
    size_t numEmails = conn->num_emails;
    size_t totalEmailBytes = calculate_total_email_bytes(conn);

    // create response string
    char response[MAX_RESPONSE_LENGTH];
    size_t responseLength = snprintf(response, sizeof(response), "+OK %zu %zu\r\n", numEmails, totalEmailBytes);

    // Trim the response if it exceeds the maximum length
    if (responseLength >= sizeof(response))
    {
        response[sizeof(response) - 1] = '\0'; // Ensure null-termination
        responseLength = sizeof(response) - 1; // Update the length
    }
    // print the response string
    send_n_data(response, responseLength, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - STAT: Stat action finished");
    return 0;
}

int list_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - LIST: List action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - LIST: LIST command in AUTHORIZATION state forbidden");
        return -1;
    }

    size_t numEmails = conn->num_emails;

    // creates the response header
    char responseHeader[MAX_RESPONSE_LENGTH];
    size_t responseLength = snprintf(responseHeader, sizeof(responseHeader), "+OK %zu messages:\r\n", numEmails);

    if (responseLength >= sizeof(responseHeader))
    {
        responseHeader[sizeof(responseHeader) - 1] = '\0'; // Ensure null-termination
        responseLength = sizeof(responseHeader) - 1;       // Update the length
    }

    // sends response header
    send_n_data(responseHeader, responseLength, dataSendingBuffer, conn);

    // loops through each email and sends its index and size
    for (size_t i = 0; i < numEmails; ++i)
    {
        char emailInfo[64];
        size_t emailInfoLength = snprintf(emailInfo, sizeof(emailInfo), "%zu %zu\r\n", i + 1, conn->mails[i].octets);
        send_n_data(emailInfo, emailInfoLength, dataSendingBuffer, conn);
    }

    // sends termination line
    send_data(DOT_RESPONSE, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - LIST: List action finished");

    return 0;
}

int retr_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    struct GlobalConfiguration *g_conf = get_global_configuration();

    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: RETR command in AUTHORIZATION state forbidden");
        return -1;
    }

    log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Retr action started");

    // gets the email index and subtracts 1 because mails are numbered
    size_t mailIndex = atoi(argument) - 1;
    // checks if the emailIndex is valid
    if (mailIndex > conn->num_emails)
    {
        // invalid email index
        send_data(ERR_INVALID_EMAIL_INDEX_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: User entered an invalid email index");
        return 1;
    }

    struct Mail *mail = &conn->mails[mailIndex];

    // constructs file path for the email to open
    char filePath[MAX_FILE_PATH];
    size_t buffer_size = sizeof(g_conf->maildir_folder) + sizeof(conn->username) + sizeof(mail->folder) + sizeof(mail->filename) + 3;

    //    /rootdir/username/maildir/mailfolder/filename
    snprintf(filePath, buffer_size, "%s/%s/%s/%s/%s", g_conf->mailroot_folder, conn->username, g_conf->maildir_folder, mail->folder, mail->filename);

    if (g_conf->transformation)
    {
        // Create pipes for communication between parent and child
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1)
        {
            log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Error creating pipe");
            return -1;
        }

        // Fork the process
        pid_t pid = fork();
        if (pid == -1)
        {
            log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Error forking process");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            return -1;
        }

        if (pid > 0) // Parent process
        {
            close(pipe_fd[1]); // Close write end of the pipe

            // sends initial response
            char initial[MAX_RESPONSE_LENGTH];
            size_t initialLength = sprintf(initial, "+OK %zu octets\r\n", mail->octets);
            send_n_data(initial, initialLength, dataSendingBuffer, conn);

            char buffer[BUFFER_SIZE];
            size_t bytesRead;

            log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Printing mail data to client");

            // Read from the pipe and send to the client with byte stuffing
            while ((bytesRead = read(pipe_fd[0], buffer, sizeof(buffer))) > 0)
            {
                // Check for lines starting with the termination octet
                char *line = buffer;
                char *nextLine;
                while ((nextLine = strstr(line, "\r\n")) != NULL)
                {
                    nextLine += 2; // Move past the CRLF

                    // Check if the line starts with the termination octet
                    if (nextLine[0] == '.' && (nextLine[1] == '\r' || nextLine[1] == '\n' || nextLine[1] == '\0'))
                    {
                        // Byte-stuff the line
                        memmove(nextLine + 1, nextLine, strlen(nextLine) + 1);
                        nextLine[0] = '.';
                    }

                    // Send the modified line to the client
                    send_n_data(line, nextLine - line, dataSendingBuffer, conn);

                    // Move to the next line
                    line = nextLine;
                }

                // Send the remaining data after byte stuffing
                send_n_data(line, bytesRead - (line - buffer), dataSendingBuffer, conn);
            }

            log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail data printed to client");

            // marks mail as RETRIEVED
            mail->status = RETRIEVED;
            log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail marked as RETRIEVED");

            send_data(DOT_RESPONSE, dataSendingBuffer, conn);

            close(pipe_fd[0]); // Close read end of the pipe

            // Wait for the child to finish
            wait(NULL);
        }
        else // Child process
        {
            close(pipe_fd[0]); // Close read end of the pipe

            // Redirect stdout to the write end of the pipe
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]); // Close write end of the pipe

            // Execute the transformation program (in this case, /bin/cat)
            // char *transformation_program = "/bin/cat";
            char *transformation_program = g_conf->transformation_bin_location;
            char *const paramList[] = {transformation_program, filePath, NULL};
            if (execve(transformation_program, paramList, NULL) == -1)
            {
                log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: execvpe failed in child process");
                exit(EXIT_FAILURE);
            }

            // The code below is unreachable if execve is successful
            log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Unreachable code in child process");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Open the file for reading
        FILE *file = fopen(filePath, "r");
        if (file == NULL)
        {
            log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Error opening mail file");
            return -1;
        }

        // sends initial response
        char initial[MAX_RESPONSE_LENGTH];
        size_t initialLength = sprintf(initial, "+OK %zu octets\r\n", mail->octets);
        send_n_data(initial, initialLength, dataSendingBuffer, conn);

        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Printing mail data to client");

        // reads the file and sends it to the client
        char buffer[BUFFER_SIZE];

        while (fgets(buffer, sizeof(buffer), file) != NULL)
        {
            // Check for lines ending with '\r\n.\r\n'
            size_t bytesRead = strlen(buffer);
            char *line = buffer;
            char *nextLine;

            while ((nextLine = strstr(line, "\r\n")) != NULL)
            {
                nextLine += 2; // Move past the CRLF

                // Check if the line ends with '\r\n.\r\n'
                if (nextLine[0] == '.' && nextLine[1] == '\r' && nextLine[2] == '\n' && (nextLine[3] == '\r' || nextLine[3] == '\n' || nextLine[3] == '\0'))
                {
                    // Byte-stuff the line
                    memmove(nextLine + 1, nextLine, strlen(nextLine) + 1);
                    nextLine[0] = '.';
                }

                // Send the modified line to the client
                send_n_data(line, nextLine - line, dataSendingBuffer, conn);

                // Move to the next line
                line = nextLine;
            }

            // Send the remaining data after byte stuffing
            send_n_data(line, bytesRead - (line - buffer), dataSendingBuffer, conn);
        }

        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail data printed to client");

        // closes file
        fclose(file);

        // marks mail as RETRIEVED
        mail->status = RETRIEVED;
        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail marked as RETRIEVED");

        send_data(DOT_RESPONSE, dataSendingBuffer, conn);
    }

    log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Retr action finished");
    return 0;
}

// sets specific Mail to state DELETED
int dele_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - DELE: Dele action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - DELE: DELE command in AUTHORIZATION state forbidden");
        return -1;
    }

    size_t mailIndex = atoi(argument) - 1;

    // checks if the emailIndex is valid
    if (mailIndex > conn->num_emails)
    {
        // invalid email index
        send_data(ERR_INVALID_EMAIL_INDEX_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - DELE: User entered invalid email index");
        return -1;
    }

    // sets the mail status to DELETED
    conn->mails[mailIndex].status = DELETED;
    log_message(logger, INFO, COMMAND_HANDLER, " - DELE: Mail marked as DELETED");

    send_data(OK_MARKED_DELETED, dataSendingBuffer, conn);

    return 0;
}

int noop_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - NOOP: Noop action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - NOOP: NOOP command in AUTHORIZATION state forbidden");
        return -1;
    }

    // simply responds +OK
    send_data(OK_RESPONSE, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - NOOP: Noop action finished");

    return 0;
}

int rset_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RSET: RSET command in AUTHORIZATION state forbidden");
        return -1;
    }
    log_message(logger, INFO, COMMAND_HANDLER, " - RSET: Rset action started");

    // loops through each email and sets its status to UNCHANGED
    for (size_t i = 0; i < conn->num_emails; ++i)
    {
        conn->mails[i].status = UNCHANGED;
    }
    log_message(logger, INFO, COMMAND_HANDLER, " - RSET: All mails marked as UNCHANGED");
    send_data(OK_RESPONSE, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - RSET: Rset action finished");
    return 0;
}

int quit_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMANDPARSER, " - QUIT: Quit action started");

    struct GlobalConfiguration *g_conf = get_global_configuration();

    // Envía la respuesta al cliente
    send_data(OK_SIGN_OFF, dataSendingBuffer, conn);

    log_message(logger, INFO, COMMAND_HANDLER, " - QUIT: Closing connection");

    // Cierra la conexión si está en estado de AUTORIZACIÓN
    if (conn->status == AUTHORIZATION)
    {
        if (close(conn->fd) == -1)
        {
            log_message(logger, ERROR, COMMAND_HANDLER, " - QUIT: Error closing connection (AUTHORIZATION state)");
            return -1;
        }
        log_message(logger, INFO, COMMAND_HANDLER, " - QUIT: Connection closed (AUTHORIZATION state)");
        return 1;
    }

    // Marca la conexión como en estado de ACTUALIZACIÓN
    conn->status = UPDATE;
    log_message(logger, INFO, CONNECTION, " - QUIT: Connection transitioned to UPDATE state");

    // Procesa cada correo electrónico
    for (size_t i = 0; i < conn->num_emails; ++i)
    {
        if (conn->mails[i].status == DELETED)
        {
            char filePath[MAX_FILE_PATH];
            size_t filePathLength = strlen(g_conf->mailroot_folder) + strlen(conn->username) + strlen(conn->username) + strlen(g_conf->maildir_folder) + strlen(conn->mails[i].folder) + strlen(conn->mails[i].filename) + 4;
            snprintf(filePath, filePathLength, "%s/%s/%s/%s/%s", g_conf->mailroot_folder, conn->username, g_conf->maildir_folder, conn->mails[i].folder, conn->mails[i].filename);
            delete_file(filePath);
        }
        else if (conn->mails[i].status == RETRIEVED)
        {
            // se fija que no este en cur
            if (!(conn->mails[i].folder[0] == 'c'))
            {
                // basedir: mailroot, username, maildir    || filename
                char filePath[MAX_FILE_PATH];
                size_t filePathLength = strlen(g_conf->mailroot_folder) + strlen(conn->username) + strlen(g_conf->maildir_folder) + 3;
                snprintf(filePath, filePathLength, "%s/%s/%s", g_conf->mailroot_folder, conn->username, g_conf->maildir_folder);
                if (move_file_new_to_cur(filePath, conn->mails[i].filename) == -1)
                {
                    log_message(logger, ERROR, CONNECTION, " - QUIT: Error moving file from new to cur");
                    return -1;
                }
                strcpy(conn->mails[i].folder, "cur");
            }
        }
        // Restablece el estado del correo a UNCHANGED
        conn->mails[i].status = UNCHANGED;
    }
    log_message(logger, INFO, CONNECTION, " - QUIT: All emails processed");

    // Cierra la conexión
    if (close(conn->fd) == -1)
    {
        log_message(logger, ERROR, CONNECTION, " - QUIT: Error closing connection (UPDATE state)");
        return -1;
    }
    log_message(logger, INFO, CONNECTION, " - QUIT: Connection closed (UPDATE state)");

    return 1;
}

int capa_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - CAPA: Capa action started");

    if (conn->status == AUTHORIZATION)
    {
        send_data(OK_CAPA_AUTHORIZED_MESSAGE, dataSendingBuffer, conn);
        log_message(logger, INFO, COMMAND_HANDLER, " - CAPA: Capa action finished in AUTHORIZATION state");
        return 0;
    }

    send_data(OK_CAPA_MESSAGE, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - CAPA: Capa action finished in TRANSACTION state");
    return 0;
}

int default_action(struct Connection *conn, struct buffer *dataSendingBuffer, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - COMMANDO NOT FOUND: Default action started, sending -ERR");
    send_data(ERR_DEFAULT_ACTION, dataSendingBuffer, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - COMMANDO NOT FOUND: Default action finished");
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
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, dataSendingBuffer, conn);
        log_message(conn->logger, ERROR, COMMANDPARSER, " - Unknown command");
    }
    else
    {
        if (!commands[command].accept_arguments(arg_buffer))
        {
            send_data(ERR_BAD_ARGUMENT_RESPONSE, dataSendingBuffer, conn);
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
    }

    parser_destroy(pop3_parser);
    log_message(conn->logger, INFO, COMMANDPARSER, " - Parser destroyed");

    return 0;
}
