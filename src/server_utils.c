#include "server_utils.h"

static bool done = false;
void sigterm_handler(const int signal)
{
    printf("\nSignal %d, cleaning up and exiting\n", signal); // make the main logger a global variable?
    done = true;
}

// Main Thread Handling Function
void pop3_handle_connection(struct Connection *conn)
{
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Executing Main Thread Handler");
    struct GlobalConfiguration *gConf = get_global_configuration();

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Using buffer size : %d", gConf->buffers_size);

    // Server Buffer Initialization
    struct buffer serverBuffer;
    buffer *pServerBuffer = &serverBuffer;
    // TODO verify this size is the correct, big mails case!
    uint8_t serverDataBuffer[gConf->buffers_size]; // This size should be subject to alteration through Configuration Server
    buffer_init(&serverBuffer, N(serverDataBuffer), serverDataBuffer);

    // Client Buffer Initialization
    struct buffer clientBuffer;
    buffer *pClientBuffer = &clientBuffer;
    // TODO verify this size is the correct
    uint8_t clientDataBuffer[gConf->buffers_size]; // This size should be subject to alteration through Configuration Server
    buffer_init(&clientBuffer, N(clientDataBuffer), clientDataBuffer);

    // Initial Salute
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Sending Initial Salute");
    send_data("+OK POP3 server ready (^-^)\r\n", pServerBuffer, conn);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;
    int totalBytesRead;

    // Maintain the call and response with the client
    do
    {
        uint8_t *basePointer = buffer_write_ptr(pClientBuffer, &availableSpace);
        uint8_t *ptr = basePointer;
        totalBytesRead = 0;

        // Construct complete commands with the segmented data received through the TCP Stream
        do
        {
            log_message(conn->logger, INFO, THREADMAINHANDLER, "Awaiting for Command");

            bytesRead = recv(conn->fd, ptr, availableSpace, 0);
            totalBytesRead += bytesRead;

            if (totalBytesRead > MAX_COMMAND_LENGTH)
            {
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Received command exceeds the maximum allowed length");
                error = true;
                break;
            }

            if (bytesRead > 0) // Some bytes read
            {
                ptr[bytesRead] = '\0'; // Null termination
                log_message(conn->logger, INFO, THREADMAINHANDLER, "Streamed by TCP: '%s'", ptr);
                buffer_write_adv(pClientBuffer, bytesRead);             // Advance buffer
                ptr = buffer_write_ptr(pClientBuffer, &availableSpace); // Update pointer

                char *commandPtr = strstr((char *)basePointer, "\r\n");
                while (commandPtr != NULL)
                {
                    *commandPtr = '\0'; // Null-terminate the command
                    log_message(conn->logger, INFO, THREADMAINHANDLER, "Command sent to parser: '%s'", basePointer);
                    parse_input(basePointer, conn, pServerBuffer);

                    // Move to the next command (if any)
                    basePointer = (uint8_t *)(commandPtr + 2);              // Move beyond the "\r\n"
                    ptr = buffer_write_ptr(pClientBuffer, &availableSpace); // Update pointer

                    // Check if there is another complete command in the remaining data
                    commandPtr = strstr((char *)basePointer, "\r\n");
                }
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
        } while (!error); // Continue reading until a complete command is processed

        log_message(conn->logger, INFO, THREADMAINHANDLER, "Resetting client buffer");
        buffer_reset(pClientBuffer);

    } while (!error && bytesRead > 0);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Freeing Resources");
    close(conn->fd);
}

// Function to send data, including support for multi-line responses
void send_data(const char *data, buffer *pBuffer, struct Connection *conn /*, bool isMultiLine*/)
{
    size_t dataLength = strlen(data);

    if (dataLength > 1024)
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

    // Check if the data is part of a multi-line response
    // if (isMultiLine)
    // {
    //     // Check if the first character of the data is the termination octet
    //     if (data[0] == '.')
    //     {
    //         // Byte-stuff the data by pre-pending the termination octet
    //         memmove(basePointer + 1, data, dataLength);
    //         basePointer[0] = '.';
    //         dataLength++;
    //     }
    // }

    // Copy the data to the buffer
    memcpy(basePointer, data, dataLength);

    // Null-terminate the data to ensure it's a valid string
    basePointer[dataLength] = '\0';

    // Advance the write pointer in the buffer by the number of bytes written
    buffer_write_adv(pBuffer, dataLength);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "SENDING DATA");

    // Send the data to the client
    sock_blocking_write(conn->fd, pBuffer);

    // Reset buffer pointers for reuse
    buffer_reset(pBuffer);
}

// Modify the function call to pass the connection struct
void *handle_connection_pthread(void *args)
{
    struct Connection *c = args;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    char threadLogFileName[30]; // Adjust the size as needed
    sprintf(threadLogFileName, "threadLog%lld.log", milliseconds);
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
    struct Connection conn;
    conn.fd = client;
    conn.addr_len = sizeof(struct sockaddr_in6);
    memcpy(&(conn.addr), caddr, sizeof(struct sockaddr_in6));
    conn.logger = clientThreadLogs; // You can set it to NULL or initialize a separate logger here

    // Call the original handling function
    pop3_handle_connection(&conn);

    // Close the client socket
    close(client);
}

// Function to send data to the client, managing buffer advancement and resetting
int send_data_udp(Logger *logger, const struct UDPClientInfo *client_info, buffer *pBuffer, char *data)
{
    log_message(logger, INFO, CONFIGTHREAD, "Responding client");

    size_t dataLength = strlen(data);

    if (dataLength > 255)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Data to be sent exceeds the maximum allowed length");
        // Handle the error, return, or exit as needed
        return -1;
    }

    size_t availableSpace;
    uint8_t *basePointer = buffer_write_ptr(pBuffer, &availableSpace);

    if (dataLength > availableSpace)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Insufficient space in the buffer to write data");
        // Handle the error, return, or exit as needed
        return -1;
    }

    // Copy the data to the buffer
    memcpy(basePointer, data, dataLength);

    // Null-terminate the data to ensure it's a valid string
    basePointer[dataLength] = '\0';

    // Advance the write pointer in the buffer by the number of bytes written
    buffer_write_adv(pBuffer, dataLength);

    // Send the data to the client
    int sent_bytes = sendto(client_info->udp_server, basePointer, dataLength, 0, (struct sockaddr *)client_info->client_addr, client_info->client_addr_len);
    if (sent_bytes == -1)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Error sending data to client");
    }

    // Reset buffer pointers for reuse
    buffer_reset(pBuffer);

    return sent_bytes;
}

// Attends and responds Configuration clients
void *handle_configuration_requests(void *arg)
{
    Logger *configurationLogger = initialize_logger("configurationServer.log");
    log_message(configurationLogger, INFO, CONFIGTHREAD, "Configuration Server is Running...");

    struct GlobalConfiguration *gConf = get_global_configuration();

    int udp_server = *((int *)arg);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Configuration Server Buffer Initialization
    struct buffer configServerBuffer;
    buffer *pConfigServerBuffer = &configServerBuffer;
    uint8_t configServerDataBuffer[gConf->buffers_size];
    buffer_init(&configServerBuffer, N(configServerDataBuffer), configServerDataBuffer);
    int received_bytes;
    size_t availableSpace = 0;

    // Create ClientInfo structure
    struct UDPClientInfo client_info = {
        .udp_server = udp_server,
        .client_addr = &client_addr,
        .client_addr_len = client_addr_len};

    while (1)
    {
        uint8_t *ptr = buffer_write_ptr(pConfigServerBuffer, &availableSpace);
        // Receive data from the client
        received_bytes = recvfrom(udp_server, ptr, availableSpace, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        log_message(configurationLogger, INFO, CONFIGTHREAD, "Client Request arrived");

        if (received_bytes == -1)
        {
            log_message(configurationLogger, ERROR, CONFIGTHREAD, "Error receiving data from client");
            break;
        }
        else if (received_bytes > 255)
        { // Invalid command
            if (send_data_udp(configurationLogger, &client_info, pConfigServerBuffer, "Command is too big") == -1)
            {
                break;
            }
        }

        // Print received data
        configServerDataBuffer[received_bytes] = '\0';
        buffer_write_adv(pConfigServerBuffer, received_bytes + 1);
        send_data_udp(configurationLogger, &client_info, pConfigServerBuffer, "Parsing gay");
        // parse_config_command(logger, &client_info, pConfigServerBuffer,); // USEN send_data_udp()!

        // EJEMPLO DE USO DE SEND DATA
        // Echo the data back to the client using the managed function
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
            struct Connection *c = malloc(sizeof(struct Connection));
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
                c->addr_len = caddrlen;
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
