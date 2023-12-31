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
#include "logger.h"
#include "parserADT.h"
#include "pop3_parser_automaton.h"
#include "args.h"
#include "server_utils.h"
#include "global_config.h"

#define MAX_USER_SIZE 10
#define DEFAULT_CAT_TRANSFORMATION "/bin/cat"
#define MAX_WAITLIST_SIZE 20

static void log_global_configuration(Logger *main_logger)
{
    struct GlobalConfiguration *global_config_instance = get_global_configuration();
    log_message(main_logger, INFO, SETUP, "<<INITIAL SERVER CONFIGURATION>>");
    log_message(main_logger, INFO, SETUP, "POP3 Address (TCP) : %s", global_config_instance->pop3_addr);
    log_message(main_logger, INFO, SETUP, "POP3 Port (TCP) : %d", global_config_instance->pop3_port);
    log_message(main_logger, INFO, SETUP, "Configuration Service Address (UDP) : %s", global_config_instance->conf_addr);
    log_message(main_logger, INFO, SETUP, "Configuration Service Port (UDP) : %d", global_config_instance->conf_port);
    log_message(main_logger, INFO, SETUP, "Users Registered (%d):", global_config_instance->numUsers);
    log_message(main_logger, INFO, SETUP, "Transformation Script (%s):", global_config_instance->transformation_script);
    for (int i = 0; i < MAX_USER_SIZE; i++)
        log_message(main_logger, INFO, SETUP, "%s", global_config_instance->users[i].name);
}

// Creates Servers
int main(const int argc, char **argv)
{
    struct GlobalConfiguration *g_conf = get_global_configuration();

    // Global Configuration Initialization
    strcpy(g_conf->logs_folder, INITIAL_LOG_FOLDER_NAME);
    g_conf->buffers_size = INITIAL_BUFFER_SIZE;
    strcpy(g_conf->mailroot_folder, INITIAL_MAILROOT_LOCATION);
    strcpy(g_conf->maildir_folder, INITIAL_MAILDIRDIR_LOCATION);
    strcpy(g_conf->authorization_token, INITIAL_AUTHORIZATION_TOKEN);
    g_conf->transformation = false;
    strcpy(g_conf->transformation_script, DEFAULT_CAT_TRANSFORMATION);

    // Logger Initialization
    char filename[45];
    sprintf(filename, "set_up_thread_%llx.log", (unsigned long long)pthread_self());
    Logger *main_logger = initialize_logger(filename);

    // Access Logger Initialization
    sprintf(filename, "user_access_%llx.log", (unsigned long long)pthread_self());
    g_conf->user_access_log = initialize_logger(filename);

    // User Input is introduced into the Global Configuration
    parse_args(argc, argv, main_logger);

    // Log Final Global Configuration
    log_global_configuration(main_logger);

    const char *err_msg;
    int ret = 0;

    // ---------------------------------------------------------------------------
    // --------------------------- MAIN POP3 SERVER ------------------------------
    // ---------------------------------------------------------------------------

    log_message(main_logger, INFO, SETUPPOP3, "Configuring Server Address");

    // Setting socket information
    struct sockaddr_in pop3_addr;
    memset(&pop3_addr, 0, sizeof(pop3_addr));
    pop3_addr.sin_family = AF_INET;
    pop3_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    pop3_addr.sin_port = htons(g_conf->pop3_port);

    // Creating POP3 TCP Socket
    log_message(main_logger, INFO, SETUPPOP3, "Creating TCP Socket");
    const int pop3_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pop3_server < 0)
    {
        err_msg = "[POP3] Unable to create server socket";
        goto finally;
    }

    setsockopt(pop3_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // Binding POP3 Server Socket
    log_message(main_logger, INFO, SETUPPOP3, "Binding Server Socket to the specified address and port");
    if (bind(pop3_server, (struct sockaddr *)&pop3_addr, sizeof(pop3_addr)) < 0)
    {
        err_msg = "[POP3] Unable to bind POP3 server socket";
        goto finally;
    }

    // Listening on POP3 Server Socket
    log_message(main_logger, INFO, SETUPPOP3, "Configuring Listening Queue for Server");
    if (listen(pop3_server, MAX_WAITLIST_SIZE) < 0)
    {
        err_msg = "[POP3] Unable to listen on POP3 server socket";
        goto finally;
    }

    log_message(main_logger, INFO, SETUPPOP3, "POP3 Server listening on TCP port %d", g_conf->pop3_port);

    // ---------------------------------------------------------------------------
    // --------------------- MAIN POP3 SERVER IPV6 VERSION -----------------------
    // ---------------------------------------------------------------------------

    // Configuring POP3 Server Address
    log_message(main_logger, INFO, SETUPPOP3, "Configuring IPV6 Server Address");
    struct sockaddr_in6 pop3_addr_6;
    memset(&pop3_addr_6, 0, sizeof(pop3_addr_6));
    pop3_addr_6.sin6_family = AF_INET6;
    pop3_addr_6.sin6_addr = in6addr_any;
    pop3_addr_6.sin6_port = htons(g_conf->pop3_port);

    // Creating POP3 TCP Socket
    log_message(main_logger, INFO, SETUPPOP3, "Creating IPV6 TCP Socket");
    const int pop3_server_6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (pop3_server_6 < 0)
    {
        err_msg = "[POP3] Unable to create server IPV6 socket";
        goto finally;
    }

    setsockopt(pop3_server_6, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1}, sizeof(int));
    setsockopt(pop3_server_6, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // Binding POP3 Server Socket
    log_message(main_logger, INFO, SETUPPOP3, "Binding Server Socket to the specified address and port");
    if (bind(pop3_server_6, (struct sockaddr *)&pop3_addr_6, sizeof(pop3_addr_6)) < 0)
    {
        err_msg = "[POP3] Unable to bind POP3 server socket";
        goto finally;
    }

    // Listening on POP3 Server Socket
    log_message(main_logger, INFO, SETUPPOP3, "Configuring Listening Queue for Server");
    if (listen(pop3_server_6, MAX_WAITLIST_SIZE) < 0)
    {
        err_msg = "[POP3] Unable to listen on POP3 server socket";
        goto finally;
    }

    log_message(main_logger, INFO, SETUPPOP3, "POP3 Server listening on TCP port %d", g_conf->pop3_port);

    // ---------------------------------------------------------------------------
    // --------------------------- CONFIGURATION SERVER --------------------------
    // ---------------------------------------------------------------------------

    // Configuring Configuration Socket for IPv4
    log_message(main_logger, INFO, SETUPCONF, "Configuring IPv4 Server Address");
    struct sockaddr_in conf_addr;
    memset(&conf_addr, 0, sizeof(conf_addr));
    conf_addr.sin_family = AF_INET;
    conf_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    conf_addr.sin_port = htons(g_conf->conf_port);

    // Creating IPv4 Configuration UDP Socket
    log_message(main_logger, INFO, SETUPCONF, "Creating IPv4 UDP Socket");
    const int conf_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (conf_server < 0)
    {
        err_msg = "[CONF] Unable to create IPv4 configuration server socket";
        goto finally;
    }

    setsockopt(conf_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // Binding IPv4 Configuration Server Socket
    log_message(main_logger, INFO, SETUPCONF, "Binding IPv4 Socket to the specified address and port");
    if (bind(conf_server, (struct sockaddr *)&conf_addr, sizeof(conf_addr)) < 0)
    {
        err_msg = "[CONF] Unable to bind IPv4 configuration server socket";
        goto finally;
    }

    log_message(main_logger, INFO, SETUPCONF, "IPv4 Configuration Server listening on UDP port %d", g_conf->conf_port);

    // ---------------------------------------------------------------------------
    // --------------------------- SIGNAL HANDLERS -------------------------------
    // ---------------------------------------------------------------------------

    log_message(main_logger, INFO, SETUP, "Configuring Signal Handlers");
    signal(SIGTERM, sigterm_handler); // Registering SIGTERM helps tools like Valgrind
    signal(SIGINT, sigterm_handler);

    // ---------------------------------------------------------------------------
    // --------------------------- THREAD CREATION -------------------------------
    // ---------------------------------------------------------------------------

    log_message(main_logger, INFO, SETUP, "Creating Threads");

    pthread_t pop3_thread_6;
    if (pthread_create(&pop3_thread_6, NULL, serve_pop3_concurrent_blocking, (void *)&pop3_server_6) != 0)
    {
        err_msg = "[POP3] Unable to create socket";
        goto finally;
    }

    pthread_t conf_thread;
    if (pthread_create(&conf_thread, NULL, handle_configuration_requests, (void *)&conf_server) != 0)
    {
        err_msg = "[CONF] Unable to create socket";
        goto finally;
    }

    if (serve_pop3_concurrent_blocking((void *)&pop3_server) != NULL)
    {
        err_msg = "[POP3] Thread for IPv4 Failed";
        goto finally;
    }

    err_msg = 0;

finally:
    if (err_msg)
    {
        log_message(main_logger, ERROR, SETUP, err_msg);
        perror("ERROR");
        ret = 1;
    }

    // Closing sockets if they were
    if (pop3_server >= 0)
    {
        close(pop3_server);
    }

    if (conf_server >= 0)
    {
        close(conf_server);
    }

    if (pop3_server_6 >= 0)
    {
        close(pop3_server_6);
    }

    for (size_t i = 0; i < g_conf->numUsers; i++)
    {
        sem_destroy(&g_conf->users[i].semaphore);
    }

    free_all_loggers();

    return ret;
}
