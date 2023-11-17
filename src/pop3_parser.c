#include "pop3_parser.h"

void parseCommand(uint8_t *commandToParse, Logger *logger)
{
    log_message(logger, INFO, COMMANDPARSER, "Arrived at Parser!");
    log_message(logger, INFO, COMMANDPARSER, "Arriving Command '%s'", (char *)commandToParse);
    log_message(logger, INFO, COMMANDPARSER, "Arriving Command Length %d", strlen((char *)commandToParse));
}
