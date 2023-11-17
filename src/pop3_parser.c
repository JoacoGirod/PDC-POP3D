#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "logger.h"

typedef struct
{
    const char *command;
    void (*handler)(const char *arg);
} CommandHandler;

void handleUser(const char *arg)
{
    if (arg == NULL)
    {
        printf("Invalid command\n");
        return;
    }
    printf("USER: %s\n", arg);
}

void handlePass(const char *arg)
{
    if (arg == NULL)
    {
        printf("Invalid command\n");
        return;
    }
    printf("PASS: %s\n", arg);
}

void handleStat(const char *arg)
{
    printf("STAT\n");
}

void handleList(const char *arg)
{
    if (arg == NULL)
    {
        printf("LIST\n");
    }
    else
    {
        printf("LIST %s\n", arg);
    }
}

void handleRetr(const char *arg)
{
    if (arg == NULL)
    {
        printf("Invalid command\n");
        return;
    }
    printf("RETR %s\n", arg);
}

void handleRset(const char *arg)
{
    printf("RSET\n");
}

void handleDele(const char *arg)
{
    if (arg == NULL)
    {
        printf("Invalid command\n");
        return;
    }
    printf("DELE %s\n", arg);
}

void handleNoop(const char *arg)
{
    printf("NOOP\n");
}

void handleQuit(const char *arg)
{
    printf("QUIT\n");
}

void handleCapa(const char *arg)
{
    printf("CAPA\n");
}

// Add handlers for other commands

void parseCommand(uint8_t *commandToParse, Logger *logger)
{
    log_message(logger, INFO, COMMANDPARSER, "Arrived at Parser!");
    log_message(logger, INFO, COMMANDPARSER, "Arriving Command '%s'", (char *)commandToParse);
    log_message(logger, INFO, COMMANDPARSER, "Arriving Command Length %d", strlen((char *)commandToParse));

    // char *token = strtok(commandToParse, "\r\n");
    // char *command = strtok(token, " ");
    // char *arg = strtok(NULL, " ");

    // CommandHandler handlers[] = {
    //     {"USER", handleUser},
    //     {"PASS", handlePass},
    //     {"STAT", handleStat},
    //     {"LIST", handleList},
    //     {"RETR", handleRetr},
    //     {"RSET", handleRset},
    //     {"DELE", handleDele},
    //     {"NOOP", handleNoop},
    //     {"QUIT", handleQuit},
    //     {"CAPA", handleCapa},
    // };

    // int numHandlers = sizeof(handlers) / sizeof(handlers[0]);

    // for (int i = 0; i < numHandlers; ++i)
    // {
    //     if (strcmp(command, handlers[i].command) == 0)
    //     {
    //         handlers[i].handler(arg);
    //         return;
    //     }
    // }

    printf("Invalid command\n");
}
