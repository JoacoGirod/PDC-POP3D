#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "buffer.h"
#include "netutils.h"
#include "tests.h"
#include "logger.h"
#include "parserADT.h"
#include "pop3_parser_automaton.h"
#include "args.h"
#include "server_utils.h"
#include "global_config.h"

static void log_global_configuration(Logger *mainLogger)
{
    struct GlobalConfiguration *global_config_instance = get_global_configuration();
    log_message(mainLogger, INFO, SETUP, "<<INITIAL SERVER CONFIGURATION>>");
    log_message(mainLogger, INFO, SETUP, "POP3 Address (TCP) : %s", global_config_instance->pop3_addr);
    log_message(mainLogger, INFO, SETUP, "POP3 Port (TCP) : %d", global_config_instance->pop3_port);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Address (UDP) : %s", global_config_instance->conf_addr);
    log_message(mainLogger, INFO, SETUP, "Configuration Service Port (UDP) : %d", global_config_instance->conf_port);
    log_message(mainLogger, INFO, SETUP, "Users Registered (%d):", global_config_instance->numUsers);
    for (int i = 0; i < 10; i++)
        log_message(mainLogger, INFO, SETUP, "%s", global_config_instance->users[i].name);
}

// Creates Servers
int main(const int argc, char **argv)
{
    struct GlobalConfiguration *gConf = get_global_configuration();
    gConf->buffers_size = INITIAL_BUFFER_SIZE;
    gConf->logs_folder = INITIAL_LOG_FOLDER_NAME;
    gConf->maildir_folder = INITIAL_MAILDIRDIR_LOCATION;
    Logger *mainLogger = initialize_logger("mainLogs.log");

    parse_args(argc, argv, mainLogger); // Function automatically sets the Global Configuration

    log_global_configuration(mainLogger);

    const char *err_msg;

    // ---------------------------------------------------------------------------
    // --------------------------- MAIN POP3 SERVER ------------------------------
    // ---------------------------------------------------------------------------

    // Configuring POP3 Server Address
    log_message(mainLogger, INFO, SETUPPOP3, "Configuring Server Address");
    struct sockaddr_in pop3_addr;
    memset(&pop3_addr, 0, sizeof(pop3_addr));
    pop3_addr.sin_family = AF_INET;

    // TODO maybe this can be transfered into the parse arguments and be directly saved in the correct type
    if (inet_pton(AF_INET, gConf->pop3_addr, &(pop3_addr.sin_addr)) <= 0)
    {
        log_message(mainLogger, ERROR, SETUPPOP3, "Invalid server address format: %s", gConf->pop3_addr);
        return 1;
    }

    pop3_addr.sin_port = htons(gConf->pop3_port);

    // Creating POP3 TCP Socket
    log_message(mainLogger, INFO, SETUPPOP3, "Creating TCP Socket");
    const int pop3_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pop3_server < 0)
    {
        err_msg = "[POP3] Unable to create server socket";
        goto finally;
    }

    // POP3 Server is configured to not wait for the last bytes transferred after the FIN in the TCP Connection
    // This allows for quick restart of the server, should only be used in development
    setsockopt(pop3_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // Binding POP3 Server Socket
    log_message(mainLogger, INFO, SETUPPOP3, "Binding Server Socket to the specified address and port");
    if (bind(pop3_server, (struct sockaddr *)&pop3_addr, sizeof(pop3_addr)) < 0)
    {
        err_msg = "[POP3] Unable to bind POP3 server socket";
        goto finally;
    }

    // Listening on POP3 Server Socket
    log_message(mainLogger, INFO, SETUPPOP3, "Configuring Listening Queue for Server");
    if (listen(pop3_server, 20) < 0)
    {
        err_msg = "Unable to listen on POP3 server socket";
        goto finally;
    }

    log_message(mainLogger, INFO, SETUPPOP3, "POP3 Server listening on TCP port %d", gConf->pop3_port);

    // ---------------------------------------------------------------------------
    // --------------------------- CONFIGURATION SERVER --------------------------
    // ---------------------------------------------------------------------------

    // Configuring Configuration Server Address
    log_message(mainLogger, INFO, SETUPCONF, "Configuring Server Address");
    struct sockaddr_in conf_addr;
    memset(&conf_addr, 0, sizeof(conf_addr));
    conf_addr.sin_family = AF_INET;

    // TODO maybe this can be transfered into the parse arguments and be directly saved in the correct type
    if (inet_pton(AF_INET, gConf->conf_addr, &(conf_addr.sin_addr)) <= 0)
    {
        log_message(mainLogger, ERROR, SETUPCONF, "Invalid configuration server address format: %s", gConf->conf_addr);
        return 1;
    }

    conf_addr.sin_port = htons(gConf->conf_port);

    // Creating Configuration UDP Socket
    log_message(mainLogger, INFO, SETUPCONF, "Creating UDP Socket");
    const int conf_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (conf_server < 0)
    {
        err_msg = "Unable to create configuration server socket";
        goto finally;
    }

    // Binding Configuration Server Socket
    log_message(mainLogger, INFO, SETUPCONF, "Binding Socket to the specified address and port");
    if (bind(conf_server, (struct sockaddr *)&conf_addr, sizeof(conf_addr)) < 0)
    {
        err_msg = "Unable to bind configuration server socket";
        goto finally;
    }

    log_message(mainLogger, INFO, SETUPCONF, "Configuration Server listening on UDP port %d", gConf->conf_port);

    // Configuring Signal Handlers
    log_message(mainLogger, INFO, SETUP, "Configuring Signal Handlers");
    signal(SIGTERM, sigterm_handler); // Registering SIGTERM helps tools like Valgrind
    signal(SIGINT, sigterm_handler);

    // Creating a thread for handling UDP requests
    pthread_t udp_thread;
    if (pthread_create(&udp_thread, NULL, handle_configuration_requests, (void *)&conf_server) != 0)
    {
        log_message(mainLogger, ERROR, SETUPCONF, "Failed to create UDP thread");
        goto finally;
    }

    err_msg = 0;
    int ret = serve_pop3_concurrent_blocking(pop3_server);

finally:
    if (err_msg)
    {
        log_message(mainLogger, ERROR, SETUP, err_msg);
        ret = 1;
    }

    // Closing sockets
    if (pop3_server >= 0)
    {
        close(pop3_server);
    }

    if (conf_server >= 0)
    {
        close(conf_server);
    }

    return ret;
}
