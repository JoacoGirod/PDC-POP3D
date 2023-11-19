// global_config.h

#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <stddef.h>
#include "logger.h"
#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 256
#define MAX_EMAILS 64
#define MAX_USERS 10

struct Users
{
    char *name;
    char *pass;
};

struct GlobalConfiguration
{
    char *pop3_addr;
    int pop3_port;
    char *conf_addr;
    int conf_port;
    struct Users users[MAX_USERS];
    size_t numUsers;
    size_t concurrent_clients;
    size_t total_clients;
    size_t bytes_transfered;
};

struct GlobalConfiguration *get_global_configuration();
void log_global_configuration(Logger *mainLogger);

#endif // GLOBAL_CONFIG_H
