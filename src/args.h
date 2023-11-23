#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdio.h>  /* for printf */
#include <stdlib.h> /* for exit */
#include <limits.h> /* LONG_MIN et al */
#include <string.h> /* memset */
#include <errno.h>
#include <getopt.h>
#include "logger.h"
#include "global_config.h"

void parse_args(const int argc, char **argv, Logger *logger);

#endif // ARGS_H
