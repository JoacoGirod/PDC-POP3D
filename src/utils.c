
#include "utils.h"

void logConfiguration(struct pop3args args, Logger *mainLogger)
{
    log_message(mainLogger, INFO, SETUP, "POP3 Address : %s", args.pop3_addr);
    log_message(mainLogger, INFO, SETUP, "POP3 Port : %d", args.pop3_port);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Address : %s", args.mng_addr);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Port : %d", args.mng_port);
    log_message(mainLogger, INFO, SETUP, "Users Registered :");
    for (int i = 0; i < 10; i++)
    {
        log_message(mainLogger, INFO, SETUP, "%s", args.users[i].name);
    }
}
