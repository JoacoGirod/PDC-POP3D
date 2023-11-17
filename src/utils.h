// utils.h

#ifndef UTILS_H
#define UTILS_H

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

// Declaration for the function
void logConfiguration(struct pop3args args, Logger *mainLogger);

#endif // UTILS_H
