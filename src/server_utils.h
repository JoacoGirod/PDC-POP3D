// server_utils.h

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include "logger.h"
#include "args.h"
#include "buffer.h"
#include "netutils.h"
#include "parser_automaton.h"
#include "parserADT.h"

#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 256
#define MAX_EMAILS 64

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

/**
 * Structure used to transport information from the distributor thread onto the client handler threads
 */
struct connection
{
    int fd;
    socklen_t addrlen;
    struct sockaddr_in6 addr;
    Logger *logger;
    struct Mail mails[MAX_EMAILS];
    size_t numEmails;
};

void send_data(const char *data, buffer *pBuffer, struct connection *conn);
int serve_pop3_concurrent_blocking(const int server);
void *handle_configuration_requests(void *arg);
void handle_client_without_threading(int client, const struct sockaddr_in6 *caddr);
void *handle_connection_pthread(void *args);
void pop3_handle_connection(struct connection *conn);
void sigterm_handler(const int signal);

#endif // SERVER_UTILS_H
