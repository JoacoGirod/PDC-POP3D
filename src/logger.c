#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include <sys/stat.h>

#define MAX_LOGGERS 10  // Adjust the maximum number of loggers as needed
Logger *initializedLoggers[MAX_LOGGERS] = {NULL};
size_t numInitializedLoggers = 0;
const char *LOG_FOLDER = "logs";  // Default logs folder

char *my_strdup(const char *str) {
    size_t len = strlen(str) + 1; // +1 for the null terminator
    char *dup_str = (char *)malloc(len);

    if (dup_str != NULL) {
        memcpy(dup_str, str, len);
    }

    return dup_str;
}

// Function to create the logs folder if it doesn't exist
void create_logs_folder() {
    struct stat st = {0};
    if (stat(LOG_FOLDER, &st) == -1) {
        mkdir(LOG_FOLDER, 0700);
    }
}

// Function to initialize the logger
Logger *initialize_logger(const char *logFileName) {
    if (numInitializedLoggers >= MAX_LOGGERS) {
        fprintf(stderr, "Cannot initialize more loggers. Maximum limit reached.\n");
        return NULL;
    }

    // Create the logs folder if it doesn't exist
    create_logs_folder();

    Logger *logger = (Logger *)malloc(sizeof(Logger));

    if (logger == NULL) {
        perror("Error allocating memory for logger");
        return NULL;
    }

    // Create the full path to the log file
    char fullLogFileName[256];  // Adjust the size as needed
    snprintf(fullLogFileName, sizeof(fullLogFileName), "%s/%s", LOG_FOLDER, logFileName);

    logger->logFileName = my_strdup(fullLogFileName);

    if (logger->logFileName == NULL) {
        perror("Error duplicating log file name");
        free(logger);
        return NULL;
    }

    initializedLoggers[numInitializedLoggers++] = logger;

    FILE *logFile = fopen(logger->logFileName, "a");

    if (logFile == NULL) {
        perror("Error opening log file");
        free(logger->logFileName);
        free(logger);
        return NULL;
    }

    // Add an initial line to the log file
    fprintf(logFile, "LOGS FOR POP3\n");

    fclose(logFile);

    return logger;
}


// Function to free resources associated with a specific logger
void free_logger(Logger *logger) {
    if (logger != NULL) {
        free((void *)logger->logFileName);
        free(logger);
    }
}

// Function to free all initialized loggers
void free_all_loggers() {
    for (size_t i = 0; i < numInitializedLoggers; ++i) {
        free_logger(initializedLoggers[i]);
    }

    // Reset the array and count
    numInitializedLoggers = 0;
}

// Log function
void log_message(Logger *logger, LogLevel level, LogComponent component, const char *format, ...) {
    if (logger == NULL || logger->logFileName == NULL) {
        fprintf(stderr, "Logger not properly initialized\n");
        return;
    }

    // Open the log file in append mode
    FILE *logFile = fopen(logger->logFileName, "a");

    if (logFile == NULL) {
        perror("Error opening log file");
        return;
    }

    // Get current time
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);

    // Print timestamp
    fprintf(logFile, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    // Print log level
    fprintf(logFile, "%s \t --- ", (level == INFO) ? "INFO" : "ERROR");

    // Print log component
    switch (component) {
        case PARSER:
            fprintf(logFile, "PARSER");
            break;
        case THREAD:
            fprintf(logFile, "THREAD");
            break;
        case THREADMAINHANDLER:
            fprintf(logFile, "MAIN THREAD HANDLER");
            break;
        case SETUP:
            fprintf(logFile, "SETUP");
            break;
        case DISTRIBUTORTHREAD:
            fprintf(logFile, "DISTRIBUTOR THREAD");
            break;
        case ITERATIVETHREAD:
            fprintf(logFile, "DISTRIBUTOR THREAD");
            break;
        default:
            fprintf(logFile, "UNKNOWN");
    }

    fprintf(logFile, "\t --- ");

    // Print log message
    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    // Print newline
    fprintf(logFile, "\n");

    // Close the log file
    fclose(logFile);
}
