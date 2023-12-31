#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "parserADT.h"
#include "pop3_functions.h"

#define OK_MARKED_DELETED "+OK Marked to be deleted. \r\n"
#define OK_SIGN_OFF "+OK POP3 server signing off.\r\n"
#define OK_RESPONSE "+OK\r\n"
#define OK_LOGGED_IN_RESPONSE "+OK Logged in.\r\n"
#define OK_CAPA_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define OK_CAPA_AUTHORIZED_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define OK_PERMISSION_GRANTED "+OK Waitings up! Permission granted.\r\n"
#define MAX_LINE_SIZE 2048 // Adjust the size as needed

#define ERR_RESPONSE "-ERR\r\n"
#define ERR_AUTH_RESPONSE "-ERR [AUTH] Authentication failed.\r\n"
#define ERR_UNKNOWN_COMMAND_RESPONSE "-ERR Unknown command.\r\n"
#define ERR_BAD_ARGUMENT_RESPONSE "-ERR Bad argument.\r\n"
#define ERR_INVALID_EMAIL_INDEX_RESPONSE "-ERR Invalid email index.\r\n"
#define ERR_DEFAULT_ACTION "-ERR Command not found.\r\n"
#define ERR_NO_USERNAME_GIVEN_RESPONSE "-ERR No username given.\r\n"
#define ERR_USER_ALREADY_LOGGED_IN "-ERR User is already logged in and performing an operation. Waiting for clearance...\r\n"
#define DOT_RESPONSE "\r\n.\r\n"
#define SIMPLE_DOT_RESPONSE ".\r\n"

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
int user_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - USER: User action started");
    // microTesting();
    // retrieve_emails(BASE_DIR, conn);

    strcpy(conn->username, argument);
    // always sends OK
    send_data(OK_RESPONSE, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - USER: User action finished");
    return 0;
}
int pass_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Pass action started");

    if (strcmp(conn->username, "") == 0)
    {
        send_data(ERR_NO_USERNAME_GIVEN_RESPONSE, &conn->info_write_buff, conn);
        log_message(logger, INFO, COMMAND_HANDLER, " - PASS: No user given");
        return -1;
    }
    struct GlobalConfiguration *gConf = get_global_configuration();

    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Searching for user to authenticate");
    // find user and see if passwords match
    for (size_t i = 0; i < gConf->numUsers; i++)
    {
        // cycle through usernames and see if they match
        if (strcmp(conn->username, gConf->users[i].name) == 0)
        {
            log_message(logger, INFO, COMMAND_HANDLER, " - PASS: User found, attempting authentication");

            // user found, check if passwords match
            if (strcmp(gConf->users[i].pass, argument) == 0)
            {
                log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Password match, waiting for semaphore");
                if (sem_trywait(&gConf->users[i].semaphore) == 0)
                {

                    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Semaphore acquired");

                    // send OK
                    send_data(OK_LOGGED_IN_RESPONSE, &conn->info_write_buff, conn);
                    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: User authenticated");
                    // set connection status as TRANSACTION (user already authorized)
                    conn->status = TRANSACTION;

                    // retrieve user's emails
                    char filePath[MAX_FILE_PATH];
                    size_t filePathLength = strlen(gConf->mailroot_folder) + strlen(conn->username) + strlen(gConf->maildir_folder) + 3;
                    snprintf(filePath, filePathLength, "%s/%s/%s", gConf->mailroot_folder, conn->username, gConf->maildir_folder);
                    filePath[filePathLength] = '\0';
                    retrieve_emails(filePath, conn);
                    log_message(gConf->user_access_log, INFO, CONNECTION, "Thread[%lld]: User '%s' logged successfully", conn->thread_number, conn->username);
                    return 1;
                }
                else
                {
                    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Semaphore is already held, waiting...");
                    send_data(ERR_USER_ALREADY_LOGGED_IN, &conn->info_write_buff, conn);
                    sem_wait(&gConf->users[i].semaphore);
                    send_data(OK_PERMISSION_GRANTED, &conn->info_write_buff, conn);

                    log_message(logger, INFO, COMMAND_HANDLER, " - PASS: Semaphore acquired");
                    return 1;
                }
            }
            else
            {
                // incorrect password
                memset(conn->username, '\0', sizeof(conn->username));
                send_data(ERR_AUTH_RESPONSE, &conn->info_write_buff, conn);
                log_message(logger, ERROR, COMMAND_HANDLER, " - PASS: Incorrect password");
                return -1;
            }
        }
    }
    // user not found
    send_data(ERR_AUTH_RESPONSE, &conn->info_write_buff, conn);
    log_message(logger, ERROR, COMMAND_HANDLER, " - PASS: User not found");
    return 0;
}

int stat_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - STAT: Stat action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
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
    send_n_data(response, responseLength, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - STAT: Stat action finished");
    return 0;
}

int list_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - LIST: List action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
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
    send_n_data(responseHeader, responseLength, &conn->info_write_buff, conn);

    // loops through each email and sends its index and size
    for (size_t i = 0; i < numEmails; ++i)
    {
        char emailInfo[64];
        size_t emailInfoLength = snprintf(emailInfo, sizeof(emailInfo), "%zu %zu\r\n", i + 1, conn->mails[i].octets);
        send_n_data(emailInfo, emailInfoLength, &conn->info_write_buff, conn);
    }

    // sends termination line
    send_data(SIMPLE_DOT_RESPONSE, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - LIST: List action finished");

    return 0;
}

// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------

int transform_completion(struct Connection *conn, char *filePath, struct Mail *mail)
{
    // Create a pipe for communication between parent and child
    Logger *logger = conn->logger;
    struct GlobalConfiguration *g_conf = get_global_configuration();
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Pipe creation failed");
        return -1;
    }

    // Fork the process
    pid_t child_pid = fork();

    if (child_pid == -1)
    {
        perror("Fork failed");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    }

    if (child_pid == 0) // Child process
    {
        // Close the writing end of the pipe
        close(pipe_fd[1]);

        // Redirect stdin to the reading end of the pipe
        dup2(pipe_fd[0], STDIN_FILENO);

        // Redirect stdout to the socket
        dup2(conn->fd, STDOUT_FILENO);

        // Close unused file descriptors
        close(pipe_fd[0]);
        close(conn->fd);

        // Execute the transformation program
        execl("/bin/sh", "sh", "-c", g_conf->transformation_script, (char *)NULL);

        // If execl fails
        log_message(logger, ERROR, COMMAND_HANDLER, "execl() failed");

        exit(EXIT_FAILURE);
    }
    else // Parent process
    {
        // Close the reading end of the pipe
        close(pipe_fd[0]);

        // Open the file for reading
        FILE *file = fopen(filePath, "r");
        if (file == NULL)
        {
            log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Error opening mail file");
            return -1;
        }

        char buffer[BUFFER_SIZE];
        size_t bytesRead = 0;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            // Handle byte stuffing for \r\n.\r\n terminations
            size_t i = 0;
            while (i < bytesRead)
            {
                if (buffer[i] == '.' && (i == 0 || buffer[i - 1] == '\n'))
                {
                    // Byte-stuffing: prepend an extra '.'
                    write(pipe_fd[1], ".", 1);
                }

                // Send the current character
                write(pipe_fd[1], &buffer[i], 1);

                if (buffer[i] == '\n' && (i == 0 || buffer[i - 1] != '\r'))
                {
                    // Send the carriage return for lines that end with '\n'
                    write(pipe_fd[1], "\r", 1);
                }

                i++;
            }
        }

        // Close the writing end of the pipe to signal the end of input to the child
        close(pipe_fd[1]);

        // Wait for the child to finish
        int status;
        waitpid(child_pid, &status, 0);

        // Close the file
        fclose(file);

        // marks mail as RETRIEVED
        mail->status = RETRIEVED;
        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail marked as RETRIEVED");

        send_data(DOT_RESPONSE, &conn->info_write_buff, conn);
    }
    return 0;
}

int retr_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    struct GlobalConfiguration *g_conf = get_global_configuration();

    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: RETR command in AUTHORIZATION state forbidden");
        return -1;
    }
    // gets the email index and subtracts 1 because mails are numbered
    int mailIndex = atoi(argument) - 1;
    // checks if the emailIndex is valid
    if ((size_t)mailIndex >= conn->num_emails || mailIndex < 0)
    {
        // invalid email index
        send_data(ERR_INVALID_EMAIL_INDEX_RESPONSE, &conn->info_write_buff, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: User entered an invalid email index");
        return 1;
    }
    // Check if the argument is a valid number
    size_t len = strlen(argument);
    size_t digit_len = strspn(argument, "0123456789");
    if (digit_len == 0 || digit_len != len)
    {
        char error_message[MAX_RESPONSE_LENGTH];
        snprintf(error_message, sizeof(error_message), "-ERR Noise after message number: %s\r\n", argument);
        send_data(error_message, &conn->info_write_buff, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Noise after message number: %s", argument);
        return 1;
    }
    log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Retr action started");

    struct Mail *mail = &conn->mails[mailIndex];

    // constructs file path for the email to open
    char filePath[MAX_FILE_PATH];
    size_t buffer_size = strlen(g_conf->maildir_folder) + strlen(g_conf->maildir_folder) + strlen(conn->username) + strlen(mail->folder) + strlen(mail->filename) + 4;
    snprintf(filePath, buffer_size, "%s/%s/%s/%s/%s", g_conf->mailroot_folder, conn->username, g_conf->maildir_folder, mail->folder, mail->filename);

    // ---------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------

    if (g_conf->transformation)
    {
        log_message(logger, DEBUG, COMMAND_HANDLER, " - RETR Transformacion!");
        // sends initial response
        char initial[MAX_RESPONSE_LENGTH];
        size_t initialLength = sprintf(initial, "+OK %zu octets\r\n", mail->octets);
        send_n_data(initial, initialLength, &conn->info_write_buff, conn);
        if (transform_completion(conn, filePath, mail) == -1)
        {
            log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: Error transforming mail");
            return -1;
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
        send_n_data(initial, initialLength, &conn->info_write_buff, conn);

        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Printing mail data to client");

        // reads the file and sends it to the client
        char buffer[BUFFER_SIZE];
        size_t bytesRead = 0;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            // Handle byte stuffing for \r\n.\r\n terminations
            size_t i = 0;
            while (i < bytesRead)
            {
                if (buffer[i] == '.' && (i == 0 || buffer[i - 1] == '\n'))
                {
                    // Byte-stuffing: prepend an extra '.'
                    send_n_data(".", 1, &conn->info_write_buff, conn);
                }

                // Send the current character
                send_n_data(&buffer[i], 1, &conn->info_write_buff, conn);

                if (buffer[i] == '\n' && (i == 0 || buffer[i - 1] != '\r'))
                {
                    // Send the carriage return for lines that end with '\n'
                    send_n_data("\r", 1, &conn->info_write_buff, conn);
                }

                i++;
            }
        }
        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail data printed to client");

        // closes file
        fclose(file);

        // marks mail as RETRIEVED
        mail->status = RETRIEVED;
        log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Mail marked as RETRIEVED");

        send_data(DOT_RESPONSE, &conn->info_write_buff, conn);
    }

    // ---------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------

    log_message(logger, INFO, COMMAND_HANDLER, " - RETR: Retr action finished");
    return 0;
}

// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------

// sets specific Mail to state DELETED
int dele_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - DELE: Dele action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - DELE: DELE command in AUTHORIZATION state forbidden");
        return -1;
    }

    int mailIndex = atoi(argument) - 1;
    // checks if the emailIndex is valid
    if ((size_t)mailIndex >= conn->num_emails || mailIndex < 0)
    {
        // invalid email index
        send_data(ERR_INVALID_EMAIL_INDEX_RESPONSE, &conn->info_write_buff, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - RETR: User entered an invalid email index");
        return 1;
    }

    // sets the mail status to DELETED
    conn->mails[mailIndex].status = DELETED;
    log_message(logger, INFO, COMMAND_HANDLER, " - DELE: Mail marked as DELETED");

    send_data(OK_MARKED_DELETED, &conn->info_write_buff, conn);

    return 0;
}

int noop_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - NOOP: Noop action started");
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
        log_message(logger, ERROR, COMMAND_HANDLER, " - NOOP: NOOP command in AUTHORIZATION state forbidden");
        return -1;
    }

    // simply responds +OK
    send_data(OK_RESPONSE, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - NOOP: Noop action finished");

    return 0;
}

int rset_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    if (conn->status == AUTHORIZATION)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
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
    send_data(OK_RESPONSE, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - RSET: Rset action finished");
    return 0;
}

int quit_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMANDPARSER, " - QUIT: Quit action started");

    struct GlobalConfiguration *g_conf = get_global_configuration();

    // Envía la respuesta al cliente
    send_data(OK_SIGN_OFF, &conn->info_write_buff, conn);

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

    for (size_t i = 0; i < g_conf->numUsers; i++)
    {
        if (strcmp(g_conf->users[i].name, conn->username) == 0)
        {
            sem_post(&g_conf->users[i].semaphore);
        }
    }

    log_message(logger, INFO, CONNECTION, " - QUIT: Connection closed (UPDATE state)");

    return 1;
}

int capa_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - CAPA: Capa action started");

    if (conn->status == AUTHORIZATION)
    {
        send_data(OK_CAPA_AUTHORIZED_MESSAGE, &conn->info_write_buff, conn);
        log_message(logger, INFO, COMMAND_HANDLER, " - CAPA: Capa action finished in AUTHORIZATION state");
        return 0;
    }

    send_data(OK_CAPA_MESSAGE, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - CAPA: Capa action finished in TRANSACTION state");
    return 0;
}

int default_action(struct Connection *conn, char *argument)
{
    Logger *logger = conn->logger;
    log_message(logger, INFO, COMMAND_HANDLER, " - COMMANDO NOT FOUND: Default action started, sending -ERR");
    send_data(ERR_DEFAULT_ACTION, &conn->info_write_buff, conn);
    log_message(logger, INFO, COMMAND_HANDLER, " - COMMANDO NOT FOUND: Default action finished");
    return 0;
}

typedef int (*command_action)(struct Connection *conn, char *argument);
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

int parse_input(const uint8_t *input, struct Connection *conn)
{
    // struct GlobalConfiguration *gConf = get_global_configuration();
    // Users array
    // gConf->users
    // Quantity of Users for cycling
    // gConf->numUsers

    // Logger conn->logger

    // send_data("Esto le reponde al usuario", &conn->info_write_buff, conn);

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
            send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
            log_message(conn->logger, ERROR, COMMANDPARSER, " - Parser error");
            return 0;
        }
    }

    char cmd_buffer[CMD_LENGTH + 1];
    char arg_buffer[ARG_MAX_LENGTH + 1];
    get_pop3_cmd(pop3_parser, cmd_buffer, CMD_LENGTH + 1);
    get_pop3_arg(pop3_parser, arg_buffer, ARG_MAX_LENGTH + 1);

    pop3_command command = get_command(cmd_buffer);
    if (command == ERROR_COMMAND)
    {
        send_data(ERR_UNKNOWN_COMMAND_RESPONSE, &conn->info_write_buff, conn);
        log_message(conn->logger, ERROR, COMMANDPARSER, " - Unknown command");
    }
    else
    {
        if (!commands[command].accept_arguments(arg_buffer))
        {
            send_data(ERR_BAD_ARGUMENT_RESPONSE, &conn->info_write_buff, conn);
            log_message(conn->logger, ERROR, COMMANDPARSER, " - Bad argument");
        }
        else
        {
            int commandRet = commands[command].action(conn, arg_buffer);
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
