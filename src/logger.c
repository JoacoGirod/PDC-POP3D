#include "logger.h"

Logger *initializedLoggers[MAX_LOGGERS] = {NULL};
size_t numInitializedLoggers = 0;
const char *LOG_FOLDER = "logs"; // Default logs folder

char *my_strdup(const char *str)
{
    size_t len = strlen(str) + 1; // +1 for the null terminator
    char *dup_str = (char *)malloc(len);

    if (dup_str != NULL)
    {
        memcpy(dup_str, str, len);
    }

    return dup_str;
}

// Function to create the logs folder if it doesn't exist
void create_logs_folder()
{
    struct GlobalConfiguration *g_conf = get_global_configuration();

    struct stat st = {0};
    if (stat(g_conf->logs_folder, &st) == -1)
    {
        mkdir(g_conf->logs_folder, 0700);
    }
}

// Function that initializes the logger
Logger *initialize_logger(const char *logFileName)
{
    struct GlobalConfiguration *g_conf = get_global_configuration();

    if (numInitializedLoggers >= MAX_LOGGERS)
    {
        fprintf(stderr, "Cannot initialize more loggers. Maximum limit reached.\n");
        return NULL;
    }

    // Create the logs folder if it doesn't exist
    create_logs_folder();

    Logger *logger = (Logger *)malloc(sizeof(Logger));

    if (logger == NULL)
    {
        perror("Error allocating memory for logger");
        return NULL;
    }

    // Create the full path to the log file
    char fullLogFileName[256]; // Adjust the size as needed

    snprintf(fullLogFileName, sizeof(fullLogFileName), "%s/%s", g_conf->logs_folder, logFileName);

    logger->logFileName = my_strdup(fullLogFileName);

    if (logger->logFileName == NULL)
    {
        perror("Error duplicating log file name");
        free(logger);
        return NULL;
    }

    initializedLoggers[numInitializedLoggers++] = logger;

    FILE *logFile = fopen(logger->logFileName, "a");

    if (logFile == NULL)
    {
        perror("Error opening log file");
        free(logger->logFileName);
        free(logger);
        return NULL;
    }

    // Add an initial line to the log file
    fprintf(logFile, "LOGS\n");

    fclose(logFile);

    return logger;
}

// Frees resources associated with a specific logger
void free_logger(Logger *logger)
{
    if (logger != NULL)
    {
        free((void *)logger->logFileName);
        free(logger);
    }
}

// Function to free all initialized loggers
void free_all_loggers()
{
    for (size_t i = 0; i < numInitializedLoggers; ++i)
    {
        free_logger(initializedLoggers[i]);
    }

    // Reset the array and count
    numInitializedLoggers = 0;
}

// Log function
void log_message(Logger *logger, LogLevel level, LogComponent component, const char *format, ...)
{
    // if (logger == NULL || logger->logFileName == NULL)
    // {
    //     fprintf(stderr, "Logger not properly initialized\n");
    //     return;
    // }

    // // Open the log file in append mode
    // FILE *logFile = fopen(logger->logFileName, "a");

    // if (logFile == NULL)
    // {
    //     perror("Error opening log file");
    //     return;
    // }

    // // Get current time
    // time_t t;
    // struct tm *tm_info;
    // time(&t);
    // tm_info = localtime(&t);

    // // Print timestamp
    // fprintf(logFile, "[%04d-%02d-%02d %02d:%02d:%02d] ",
    //         tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
    //         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    // // Print log level
    // fprintf(logFile, "%s \t --- ", (level == INFO) ? "INFO" : "ERROR");

    // // Print log component
    // switch (component)
    // {
    // case COMMAND_HANDLER:
    //     fprintf(logFile, "COMMAND HANDLER");
    //     break;
    // case CONNECTION:
    //     fprintf(logFile, "CONNECTION");
    //     break;
    // case COMMANDPARSER:
    //     fprintf(logFile, "COMMAND PARSER");
    //     break;
    // case ARGPARSER:
    //     fprintf(logFile, "ARGUMENTS PARSER");
    //     break;
    // case THREAD:
    //     fprintf(logFile, "THREAD");
    //     break;
    // case CONFIGTHREAD:
    //     fprintf(logFile, "CONFIGURATION THREAD");
    //     break;
    // case THREADMAINHANDLER:
    //     fprintf(logFile, "MAIN THREAD HANDLER");
    //     break;
    // case SETUP:
    //     fprintf(logFile, "SETUP");
    //     break;
    // case SETUPPOP3:
    //     fprintf(logFile, "SETUP POP3");
    //     break;
    // case SETUPCONF:
    //     fprintf(logFile, "SETUP CONF");
    //     break;
    // case DISTRIBUTORTHREAD:
    //     fprintf(logFile, "DISTRIBUTOR THREAD");
    //     break;
    // case ITERATIVETHREAD:
    //     fprintf(logFile, "DISTRIBUTOR THREAD");
    //     break;
    // default:
    //     fprintf(logFile, "UNKNOWN");
    // }

    // fprintf(logFile, "\t --- ");

    // // Print log message
    // va_list args;
    // va_start(args, format);
    // vfprintf(logFile, format, args);
    // va_end(args);

    // // Print newline
    // fprintf(logFile, "\n");

    // // Close the log file
    // fclose(logFile);
}
