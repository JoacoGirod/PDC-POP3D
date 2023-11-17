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
#include "pop3_parser.h"
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

    do
    {
        // Function buffer_write_ptr() returns the pointer to the writable area and the amount of bytes I can write
        uint8_t *ptr = buffer_write_ptr(pServerBuffer, &availableSpace);

        // Receive the information sent by the client and save it in the buffer
        // TODO add relevant flags to this call
        bytesRead = recv(conn->fd, ptr, availableSpace, 0);

        // TODO this could fail if the TCP Stream doesnt have the complete command, it should have a cumulative variable and reset it when a command is succesfully parsed
        if (bytesRead > 0)
        {
            // Check if the received command exceeds the maximum length defined in the RFC Extension
            if (bytesRead > 255)
            {
                // TODO improve error handling here
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Received command exceeds the maximum allowed length");
                error = true; // goto finally?
                break;
            }
            // Null-terminate the received data to ensure it's a valid string
            ptr[bytesRead] = '\0';

            // Advance the write pointer in the buffer by the number of bytes received
            buffer_write_adv(pServerBuffer, bytesRead);

            parseCommand(ptr, conn->logger);
        }
        else if (bytesRead == 0)
        {
            log_message(conn->logger, INFO, THREADMAINHANDLER, "Connection close by the client");
            break;
        }
        else
        {
            // Handle the error
            log_message(conn->logger, ERROR, THREADMAINHANDLER, "recv() failed");
            error = true; // goto finally?
            break;
        }
    } while (!error && bytesRead > 0);

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

    unsigned port = args.pop3_port; // Standard Port defined in class

    log_message(mainLogger, INFO, SETUP, "Configuring Server Address");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    // Use the user-provided address or default to INADDR_ANY
    if (inet_pton(AF_INET, args.pop3_addr, &(addr.sin_addr)) <= 0)
    {
        log_message(mainLogger, ERROR, SETUP, "Invalid address format: %s", args.pop3_addr);
        return 1;
    }

    addr.sin_port = htons(port);

    const char *err_msg;

    log_message(mainLogger, INFO, SETUP, "Creating TCP Socket");
    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0)
    {
        err_msg = "Unable to create socket";
        goto finally;
    }

    // Server is configured to not wait for the last bytes transfered after the FIN in the TCP Connection
    // This allows for quick restart of the server, should only be used in development
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    log_message(mainLogger, INFO, SETUP, "Binding the Socket to the specified port and address");
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        err_msg = "Unable to bind socket";
        goto finally;
    }

    log_message(mainLogger, INFO, SETUP, "Configuring Listening Queue");
    // TODO : change this number from 20 to 500 to allow for more concurrent connections, test what happens with 3
    if (listen(server, 20) < 0)
    {
        err_msg = "Unable to listen";
        goto finally;
    }

    log_message(mainLogger, INFO, SETUP, "Configuring Signal Handlers");
    signal(SIGTERM, sigterm_handler); // Registering SIGTERM helps tools like Valgrind
    signal(SIGINT, sigterm_handler);

    log_message(mainLogger, INFO, SETUP, "POP3 Server listening on TCP port %d", port);

    err_msg = 0;
    int ret = serve_pop3_concurrent_blocking(server);

finally:
    if (err_msg)
    {
        log_message(mainLogger, ERROR, SETUP, err_msg);
        ret = 1;
    }
    if (server >= 0)
    {
        close(server);
    }
    return ret;
}
