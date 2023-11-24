#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include "logger.h"
#include "global_config.h"
#include <semaphore.h>

void parse_args(const int argc, char **argv, Logger *logger);

#endif // ARGS_H
