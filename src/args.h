#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include "logger.h"
#include "global_config.h"

void parse_args(const int argc, char **argv, Logger *logger);

#endif // ARGS_H
