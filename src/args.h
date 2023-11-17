#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include "logger.h"

#define MAX_USERS 10

struct users
{
    char *name;
    char *pass;
};

struct pop3args
{
    char *pop3_addr;
    unsigned short pop3_port;
    char *mng_addr;
    unsigned short mng_port;
    bool disectors_enabled;
    struct users users[MAX_USERS];
};

void parse_args(const int argc, char **argv, struct pop3args *args, Logger *logger);

#endif // ARGS_H
