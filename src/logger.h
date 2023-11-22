// logger.h

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "global_config.h"
#define MAX_LOGGERS 15

typedef enum
{
    INFO,
    ERROR,
    DEBUG
} LogLevel;

typedef enum
{
    COMMANDPARSER,
    CONFIGPARSER,
    COMMAND_HANDLER,
    ARGPARSER,
    CONFIGTHREAD,
    THREAD,
    THREADMAINHANDLER,
    SETUP,
    SETUPPOP3,
    SETUPCONF,
    DISTRIBUTORTHREAD,
    ITERATIVETHREAD,
    CONNECTION
} LogComponent;

// Function to initialize the logger
Logger *initialize_logger(const char *logFileName);

// Function to free resources associated with a specific logger
void free_logger(Logger *logger);

// Function to free all initialized loggers
void free_all_loggers();

// Example usage
// log_message(mainLogger, ERROR, PARSER, "Recv Failed %d", 124);
// log_message(mainLogger, INFO, THREAD, "Connection established");

// Log function
void log_message(Logger *logger, LogLevel level, LogComponent component, const char *format, ...);

#endif // LOGGER_H
