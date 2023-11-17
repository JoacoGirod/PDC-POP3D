#include "pop3_parser.h"







void parseCommand(uint8_t * commandToParse, Logger* logger){
    log_message(logger, INFO, PARSER, "Arrived at Parser!");
    log_message(logger, INFO, PARSER, "Arriving Command '%s'", (char*) commandToParse);
    log_message(logger, INFO, PARSER, "Arriving Command Length %d", strlen((char*) commandToParse));


}


