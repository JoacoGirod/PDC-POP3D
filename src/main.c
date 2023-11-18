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
#include "utils.h"

void microTesting()
{
    // Used for testing
}

static bool done = false;
static void sigterm_handler(const int signal)
{
    printf("\nSignal %d , cleaning up and exiting\n", signal); // make the main logger a global variable?
    done = true;
}

/**
 * Structure used to transport information from the distributor thread onto the client handler threads
 */
struct connection
{
    int fd;
    socklen_t addrlen;
    struct sockaddr_in6 addr;
    Logger *logger;
};

/**
 * Main Handling Function for incoming requests
 */
static void pop3_handle_connection(const struct connection *conn)
{
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Executing Main Thread Handler");
    // Buffer Initialization
    struct buffer serverBuffer;
    buffer *pServerBuffer = &serverBuffer;
    // TODO verify this size is the correct, big mails case!
    uint8_t serverDataBuffer[2048];
    buffer_init(&serverBuffer, N(serverDataBuffer), serverDataBuffer);

    // Initial State: Salute
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Sending Initial Salute");
    memcpy(serverDataBuffer, "+OK POP3 server ready (^-^)\r\n", 30);
    buffer_write_adv(pServerBuffer, 30);
    sock_blocking_write(conn->fd, pServerBuffer);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;
    int totalBytesRead;

    do
    {
        // Function buffer_write_ptr() returns the pointer to the writable area and the amount of bytes I can write
        uint8_t *basePointer = buffer_write_ptr(pServerBuffer, &availableSpace);
        uint8_t *ptr = basePointer;
        totalBytesRead = 0;

        do
        {
            // Receive the information sent by the client and save it in the buffer
            // TODO add relevant flags to this call
            bytesRead = recv(conn->fd, ptr, availableSpace, 0);
            totalBytesRead = +bytesRead;

            // Check if the received command exceeds the maximum length defined in the RFC Extension
            if (totalBytesRead > 255)
            {
                // TODO improve error handling here
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Received command exceeds the maximum allowed length");
                error = true; // goto finally?
                break;
            }

            if (bytesRead > 0)
            {
                // Null-terminate the received data to ensure it's a valid string
                ptr[bytesRead] = '\0';
                // Advance the write pointer in the buffer by the number of bytes received
                buffer_write_adv(pServerBuffer, bytesRead);
                ptr = buffer_write_ptr(pServerBuffer, &availableSpace);
            }
            else if (bytesRead == 0)
            {
                log_message(conn->logger, INFO, THREADMAINHANDLER, "Connection close by the client");
                break;
            }
            else
            {
                // Handle the error
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Function recv failed");
                error = true; // goto finally?
                break;
            }
        } while (strstr((char *)basePointer, "\r\n") == NULL); // Checking if the command is unfinished

        if (!error)
        {
            log_message(conn->logger, INFO, THREADMAINHANDLER, "Command sent to parser (should be complete): '%s'", basePointer);
            parse_input(basePointer, conn->logger);
        }

    } while (!error && bytesRead > 0 /*&& !finished*/);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Freeing Resourcers");
    close(conn->fd);
}

// Modify the function call to pass the connection struct
static void *handle_connection_pthread(void *args)
{
    struct connection *c = args;

    char threadLogFileName[20]; // Adjust the size as needed
    sprintf(threadLogFileName, "threadLog%d.log", c->fd);
    // Initialize the logger for the client thread
    Logger *clientThreadLogs = initialize_logger(threadLogFileName);

    log_message(clientThreadLogs, INFO, THREAD, "Executing Thread Routine");

    // Session Struct could be initialized here or inside the thread
    // Initializing it here would make it easier to free the resources
    // Session Attributes: state, username, directory...
    // Connection could have Session nested

    // Add the Logger to the connection struct
    c->logger = clientThreadLogs;

    pthread_detach(pthread_self());
    pop3_handle_connection(c);
    free(args);
    return 0;
}

// Function to handle the client without threading
static void handle_client_without_threading(int client, const struct sockaddr_in6 *caddr)
{
    // Create a unique log file name for the thread
    char threadLogFileName[20]; // Adjust the size as needed
    sprintf(threadLogFileName, "threadLog%d.log", client);

    // Initialize the logger with the thread-specific log file
    Logger *clientThreadLogs = initialize_logger(threadLogFileName);

    log_message(clientThreadLogs, ERROR, ITERATIVETHREAD, "Iterative thread running");

    // Create a connection struct with the necessary information
    struct connection conn;
    conn.fd = client;
    conn.addrlen = sizeof(struct sockaddr_in6);
    memcpy(&(conn.addr), caddr, sizeof(struct sockaddr_in6));
    conn.logger = clientThreadLogs; // You can set it to NULL or initialize a separate logger here

    // Call the original handling function
    pop3_handle_connection(&conn);

    // Close the client socket
    close(client);
}

// Function to handle UDP requests
#include <unistd.h>

void *handle_configuration_requests(void *arg)
{
    Logger *configurationLogger = initialize_logger("configurationServer.log");
    log_message(configurationLogger, INFO, CONFIGTHREAD, "Configuration Server is Running...");

    int udp_server = *((int *)arg);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    char buffer[1024];
    int received_bytes;

    while (1)
    {
        // Receive data from the client
        received_bytes = recvfrom(udp_server, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received_bytes == -1)
        {
            log_message(configurationLogger, ERROR, CONFIGTHREAD, "Error receiving data from client");
            break;
        }

        // Print received data
        buffer[received_bytes] = '\0';
        log_message(configurationLogger, INFO, CONFIGTHREAD, "Received message from client: %s", buffer);

        // Echo the data back to the client
        if (sendto(udp_server, buffer, received_bytes, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
        {
            log_message(configurationLogger, ERROR, CONFIGTHREAD, "Error sending data to client");
            break;
        }
    }

    return NULL;
}

/**
 * Attends clients and assigns unique blocking threads to each one
 */
int serve_pop3_concurrent_blocking(const int server)
{
    Logger *distributorThreadLogger = initialize_logger("distributorThreadLogs.log");
    log_message(distributorThreadLogger, INFO, DISTRIBUTORTHREAD, "Awaiting for connections...");
    for (; !done;)
    {
        struct sockaddr_in6 caddr;
        socklen_t caddrlen = sizeof(caddr);
        // Wait for a client to connect
        const int client = accept(server, (struct sockaddr *)&caddr, &caddrlen);
        if (client < 0)
        {
            log_message(distributorThreadLogger, ERROR, DISTRIBUTORTHREAD, "Unable to accept incoming socket");
        }
        else
        {
            // TODO(juan): limitar la cantidad de hilos concurrentes
            struct connection *c = malloc(sizeof(struct connection));
            if (c == NULL)
            {
                // We transition into a non-threaded manner
                log_message(distributorThreadLogger, INFO, DISTRIBUTORTHREAD, "Memory Allocation failed, transitioning into non-threaded manner");
                handle_client_without_threading(client, &caddr);
            }
            else
            {
                pthread_t tid;
                c->fd = client;
                c->addrlen = caddrlen;
                memcpy(&(c->addr), &caddr, caddrlen);
                log_message(distributorThreadLogger, INFO, DISTRIBUTORTHREAD, "Attempting to create a Thread");
                if (pthread_create(&tid, 0, handle_connection_pthread, c))
                {
                    free(c);
                    // We transition into a non-threaded manner
                    log_message(distributorThreadLogger, INFO, DISTRIBUTORTHREAD, "Thread Creation Failed, transitioning into non-threaded manner");
                    handle_client_without_threading(client, &caddr);
                }
            }
        }
    }
    return 0;
}

int main(const int argc, char **argv)
{
    Logger *mainLogger = initialize_logger("mainLogs.log");

    struct pop3args args;
    parse_args(argc, argv, &args, mainLogger);

    logConfiguration(args, mainLogger);

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
    if (inet_pton(AF_INET, args.pop3_addr, &(pop3_addr.sin_addr)) <= 0)
    {
        log_message(mainLogger, ERROR, SETUPPOP3, "Invalid server address format: %s", args.pop3_addr);
        return 1;
    }

    pop3_addr.sin_port = htons(args.pop3_port);

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

    log_message(mainLogger, INFO, SETUPPOP3, "POP3 Server listening on TCP port %d", args.pop3_port);

    // ---------------------------------------------------------------------------
    // --------------------------- CONFIGURATION SERVER --------------------------
    // ---------------------------------------------------------------------------

    // Configuring Configuration Server Address
    log_message(mainLogger, INFO, SETUPCONF, "Configuring Server Address");
    struct sockaddr_in conf_addr;
    memset(&conf_addr, 0, sizeof(conf_addr));
    conf_addr.sin_family = AF_INET;

    // TODO maybe this can be transfered into the parse arguments and be directly saved in the correct type
    if (inet_pton(AF_INET, args.conf_addr, &(conf_addr.sin_addr)) <= 0)
    {
        log_message(mainLogger, ERROR, SETUPCONF, "Invalid configuration server address format: %s", args.conf_addr);
        return 1;
    }

    conf_addr.sin_port = htons(args.conf_port);

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

    log_message(mainLogger, INFO, SETUPCONF, "Configuration Server listening on UDP port %d", args.conf_port);

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
