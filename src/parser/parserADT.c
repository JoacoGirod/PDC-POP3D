#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "parserADT.h"

#define CAPA_MESSAGE "+OK\r\nCAPA\r\nUSER\r\PIPELINING\r\n.\r\n"
#define BASE_DIR "mails"
#define MAX_FILE_PATH MAX_FILENAME_LENGTH + 10
#define MAX_COMMAND_LENGTH 255

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

//aux function for STAT command action
size_t calculate_total_email_bytes(struct connection *conn) {
    size_t totalBytes = 0;
    
    for (size_t i = 0; i < conn->numEmails; ++i) {
        totalBytes += conn->mails[i].octets;
    }
    
    return totalBytes;
}


// --------------------------------------------- ACTIONS ---------------------------------------------

int user_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    send_data("+OK read USER action \r\n", dataSendingBuffer, conn);
    return 0;
}
int pass_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    send_data("+OK read PASS action \r\n", dataSendingBuffer, conn);
    return 0;
}

int stat_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    //calculates number of emails and total email bytes
    size_t numEmails = conn->numEmails;
    size_t totalEmailBytes = calculate_total_email_bytes(conn);

    //create response string
    char response[256];
    snprintf(response, sizeof(response), "+OK %zu %zu\r\n", numEmails, totalEmailBytes);

    //print the response string
    send_data(response, dataSendingBuffer, conn);

    return 0;
}

int list_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    size_t numEmails = conn->numEmails;

    //creates the response header
    char responseHeader[64];
    snprintf(responseHeader, sizeof(responseHeader), "+OK %zu messages:\r\n", numEmails);

    //sends response header
    send_data(responseHeader, dataSendingBuffer, conn);

    //loops through each email and sends its index and size
    for (size_t i = 0; i < numEmails; ++i) {
        char emailInfo[64];
        snprintf(emailInfo, sizeof(emailInfo), "%zu %zu\r\n", i + 1, conn->mails[i].octets);
        send_data(emailInfo, dataSendingBuffer, conn);
    }

    //sends termination line
    send_data(".\r\n", dataSendingBuffer, conn);

    return 0;
}

int retr_action(struct connection *conn, struct buffer *dataSendingBuffer, size_t mailIndex)
{
    //checks if the emailIndex is valid
    if (mailIndex >= conn->numEmails || mailIndex <= 0) {
        //invalid email index
        send_data("-ERR Invalid email index\r\n", dataSendingBuffer, conn);
        return 1;
    }

    struct Mail *mail = &conn->mails[mailIndex];

    //constructs file path
    char filePath[MAX_FILE_PATH];
    snprintf(filePath, sizeof(filePath), "%s/%s/%s", BASE_DIR, mail->folder, mail->filename);

    // Open the file for reading
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("Error opening mail file");
        return -1;  // Or handle the error appropriately
    }

    //sends initial response
    char initial[64]; 
    sprintf(initial, "+OK %zu octets\r\n", mail->octets);
    send_data(initial, dataSendingBuffer, conn);

    //reads and sends file contents
    char buffer[1024]; 
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        fwrite(buffer, 1, bytesRead, dataSendingBuffer->write);
    }

    //closes file
    fclose(file);

    //marks mail as RETRIEVED
    mail->status = RETRIEVED;

    send_data(".\r\n", dataSendingBuffer, conn);

    return 0;
}

//sets specific Mail to state DELETED
int dele_action(struct connection *conn, struct buffer *dataSendingBuffer, size_t mailIndex)
{
    //checks if the emailIndex is valid
    if (mailIndex >= conn->numEmails || mailIndex <= 0) {
        //invalid email index
        send_data("-ERR Invalid email index\r\n", dataSendingBuffer, conn);
        return 1;
    }

    //sets the mail status to DELETED
    conn->mails[mailIndex].status = DELETED;

    send_data("+OK Marked to be deleted. \r\n", dataSendingBuffer, conn);

    return 0;
}

int noop_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    //simply responds +OK
    send_data("+OK\r\n", dataSendingBuffer, conn);

    return 0;
}

int rset_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    //loops through each email and sets its status to UNCHANGED
    for (size_t i = 0; i < conn->numEmails; ++i) {
        conn->mails[i].status = UNCHANGED;
    }
    send_data("+OK \r\n", dataSendingBuffer, conn);

    return 0;
}

int quit_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    send_data("+OK Logging out. \r\n", dataSendingBuffer, conn);
    close(conn->fd);

    return 1;
}

int capa_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    send_data(CAPA_MESSAGE, dataSendingBuffer, conn);

    return 0;
}

int default_action(struct connection *conn, struct buffer *dataSendingBuffer)
{
    // char initial[MAX_COMMAND_LENGTH]; 
    // sprintf(initial, "-ERR Unknown command %s\r\n", command); 
    send_data("-ERR Unknown command \r\n", dataSendingBuffer, conn);

    return 0;
}

typedef int (*command_action)(struct connection *conn, struct buffer *dataSendingBuffer);
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

int parse_input(const uint8_t *input, struct connection *conn, struct buffer *dataSendingBuffer)
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
        send_data("-ERR Unknown command\r\n", dataSendingBuffer, conn);
    }
    if (!commands[command].accept_arguments(arg_buffer))
    {
        send_data("-ERR Bad argument\r\n", dataSendingBuffer, conn);
    }
    else
    {
        int commandRet = commands[command].action(conn, dataSendingBuffer);
        if (commandRet == -1)
        {
            printf("Error executing command\n");
        }
    }

    parser_destroy(pop3_parser);

    return 0;
}