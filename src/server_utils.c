#include "server_utils.h"

static bool done = false;
void sigterm_handler(const int signal)
{
    printf("\nSignal %d, cleaning up and exiting\n", signal); // make the main logger a global variable?
    done = true;
}

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

// Function to send data
void send_data(const char *data, buffer *pBuffer, const struct connection *conn)
{
    size_t dataLength = strlen(data);

    if (dataLength > 255)
    {
        log_message(conn->logger, ERROR, THREADMAINHANDLER, "Data to be sent exceeds the maximum allowed length");
        // Handle the error, return, or exit as needed
        return;
    }

    size_t availableSpace;
    uint8_t *basePointer = buffer_write_ptr(pBuffer, &availableSpace);

    if (dataLength > availableSpace)
    {
        log_message(conn->logger, ERROR, THREADMAINHANDLER, "Insufficient space in the buffer to write data");
        // Handle the error, return, or exit as needed
        return;
    }

    // Copy the data to the buffer
    memcpy(basePointer, data, dataLength);

    // Null-terminate the data to ensure it's a valid string
    basePointer[dataLength] = '\0';

    // Advance the write pointer in the buffer by the number of bytes written
    buffer_write_adv(pBuffer, dataLength);

    // Send the data to the client
    sock_blocking_write(conn->fd, pBuffer);

    // Reset buffer pointers for reuse
    buffer_reset(pBuffer);
}

// Main Thread Handling Function
void pop3_handle_connection(const struct connection *conn)
{
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Executing Main Thread Handler");

    // Server Buffer Initialization
    struct buffer serverBuffer;
    buffer *pServerBuffer = &serverBuffer;
    // TODO verify this size is the correct, big mails case!
    uint8_t serverDataBuffer[2048]; // This size should be subject to alteration through Configuration Server
    buffer_init(&serverBuffer, N(serverDataBuffer), serverDataBuffer);

    // Client Buffer Initialization
    struct buffer clientBuffer;
    buffer *pClientBuffer = &clientBuffer;
    // TODO verify this size is the correct
    uint8_t clientDataBuffer[512]; // This size should be subject to alteration through Configuration Server
    buffer_init(&clientBuffer, N(clientDataBuffer), clientDataBuffer);

    // Initial Salute
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Sending Initial Salute");
    send_data("+OK POP3 server ready (^-^)\r\n", pServerBuffer, conn);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;
    int totalBytesRead;

    // Mantain the call and response with the client
    do
    {
        uint8_t *basePointer = buffer_write_ptr(pClientBuffer, &availableSpace);
        uint8_t *ptr = basePointer;
        totalBytesRead = 0;

        // Construct complete commands with the segmented data received through the TCP Stream
        do
        {
            // TODO add relevant flags to this call
            bytesRead = recv(conn->fd, ptr, availableSpace, 0);
            totalBytesRead = +bytesRead;

            if (totalBytesRead > 255) // Max Length defined in RFC Extension
            {
                // TODO improve error handling here
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Received command exceeds the maximum allowed length");
                error = true;
                break;
            }

            if (bytesRead > 0) // Some bytes read
            {
                ptr[bytesRead] = '\0';                                  // Null termination
                buffer_write_adv(pClientBuffer, bytesRead);             // Advance buffer
                ptr = buffer_write_ptr(pClientBuffer, &availableSpace); // Update pointer
            }
            else if (bytesRead == 0) // Connection loss
            {
                log_message(conn->logger, INFO, THREADMAINHANDLER, "Connection closed by the client");
                break;
            }
            else // Function failed
            {
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Function recv failed");
                error = true;
                break;
            }
        } while (strstr((char *)basePointer, "\r\n") == NULL && !error); // Checking if the command is unfinished

        if (!error)
        {
            log_message(conn->logger, INFO, THREADMAINHANDLER, "Command sent to parser (should be complete): '%s'", basePointer);
            parse_input(basePointer, conn->logger);
        }

        buffer_reset(pClientBuffer);

    } while (!error && bytesRead > 0);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Freeing Resources");
    close(conn->fd);
}

// Modify the function call to pass the connection struct
void *handle_connection_pthread(void *args)
{
    struct connection *c = args;

    char threadLogFileName[20]; // Adjust the size as needed
    sprintf(threadLogFileName, "threadLog%d.log", c->fd);
    // Initialize the logger for the client thread
    Logger *clientThreadLogs = initialize_logger(threadLogFileName);

    log_message(clientThreadLogs, INFO, THREAD, "Executing Thread Routine");

    // Add the Logger to the connection struct
    c->logger = clientThreadLogs;

    pthread_detach(pthread_self());
    pop3_handle_connection(c);
    free(args);
    return 0;
}

// Function to handle the client without threading
void handle_client_without_threading(int client, const struct sockaddr_in6 *caddr)
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

// Attends and responds Configuration clients
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

// Attends POP3 clients and assigns unique blocking threads to each one
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
