// server_utils.h

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include "logger.h"
#include "args.h"
#include "buffer.h"
#include "netutils.h"
#include "parser_automaton.h"
#include "pop3_parserADT.h"
#include "config_parserADT.h"

#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 256
#define MAX_EMAILS 64
#define MAX_USERNAME_LENGTH 255
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LENGTH 255 // Max Length defined in RFC Extension
#define MAX_CONCURRENT_USERS 500

enum ConnectionStatus
{
    AUTHORIZATION,
    TRANSACTION,
    UPDATE
};

enum MailStatus
{
    UNCHANGED,
    DELETED,
    RETRIEVED
};

struct Mail
{
    int index;
    size_t octets;
    char filename[MAX_FILENAME_LENGTH];
    char folder[4]; // Can only be tmp, cur or new
    enum MailStatus status;
};

// Structure to hold client information
struct UDPClientInfo
{
    int udp_server;
    struct sockaddr_in *client_addr;
    socklen_t client_addr_len;
    Logger *logger;
};

// typedef struct UDPClientInfo *pUDPClientInfo;

/**
 * Structure used to transport information from the distributor thread onto the client handler threads
 */
struct Connection
{
    int fd;
    socklen_t addr_len;
    struct sockaddr_in6 addr;
    Logger *logger;
    struct Mail mails[MAX_EMAILS];
    size_t num_emails;
    char username[MAX_USERNAME_LENGTH];
    enum ConnectionStatus status;
    uint8_t read_buff[BUFFER_SIZE];
    uint8_t write_buff[BUFFER_SIZE];
    uint8_t file_buff[BUFFER_SIZE];
    buffer info_file_buff;
    buffer info_read_buff;
    buffer info_write_buff;
};

void send_data(const char *data, buffer *pBuffer, struct Connection *conn);
void send_n_data(const char *data, size_t length, struct buffer *p_buffer, struct Connection *conn);

void *serve_pop3_concurrent_blocking(void *server_ptr);
void *handle_configuration_requests(void *arg);
void handle_client_without_threading(int client, const struct sockaddr_in6 *caddr);
void *handle_connection_pthread(void *args);
void pop3_handle_connection(struct Connection *conn);
void sigterm_handler(const int signal);
int send_data_udp(Logger *logger, const struct UDPClientInfo *client_info, char *data);

#endif // SERVER_UTILS_H
