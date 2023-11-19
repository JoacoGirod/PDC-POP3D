#include "global_config.h"

// Global instance of the configuration struct
static struct GlobalConfiguration global_config_instance;

struct GlobalConfiguration *get_global_configuration()
{
    return &global_config_instance;
}

void log_global_configuration(Logger *mainLogger)
{
    log_message(mainLogger, INFO, SETUP, "<<INITIAL SERVER CONFIGURATION>>");
    log_message(mainLogger, INFO, SETUP, "POP3 Address (TCP) : %s", global_config_instance.pop3_addr);
    log_message(mainLogger, INFO, SETUP, "POP3 Port (TCP) : %d", global_config_instance.pop3_port);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Address (UDP) : %s", global_config_instance.conf_addr);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Port (UDP) : %d", global_config_instance.conf_port);
    log_message(mainLogger, INFO, SETUP, "Users Registered (%d):", global_config_instance.numUsers);
    for (int i = 0; i < 10; i++)
        log_message(mainLogger, INFO, SETUP, "%s", global_config_instance.users[i].name);
}
