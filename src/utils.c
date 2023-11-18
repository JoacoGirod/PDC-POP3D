
#include "utils.h"

void logConfiguration(struct pop3args args, Logger *mainLogger)
{
    log_message(mainLogger, INFO, SETUP, "<<INITIAL SERVER CONFIGURATION>>");
    log_message(mainLogger, INFO, SETUP, "POP3 Address (TCP) : %s", args.pop3_addr);
    log_message(mainLogger, INFO, SETUP, "POP3 Port (TCP) : %d", args.pop3_port);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Address (UDP) : %s", args.conf_addr);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Port (UDP) : %d", args.conf_port);
    log_message(mainLogger, INFO, SETUP, "Users Registered :");
    for (int i = 0; i < 10; i++)
        log_message(mainLogger, INFO, SETUP, "%s", args.users[i].name);
}
