#include "server_utils.h"

static bool done = false;
void sigterm_handler(const int signal)
{
    printf("\nSignal %d, cleaning up and exiting\n", signal); // make the main logger a global variable?
    done = true;
}

// Attends POP3 clients and assigns unique blocking threads to each one
void *serve_pop3_concurrent_blocking(void *server_ptr)
{
    int server = *((int *)server_ptr);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    char thread_log_file_name[50]; // Adjust the size as needed
    sprintf(thread_log_file_name, "distributorThreadLogs%lld.log", milliseconds);

    Logger *distributor_thread_logger = initialize_logger(thread_log_file_name);

    log_message(distributor_thread_logger, INFO, DISTRIBUTORTHREAD, "Awaiting for connections...");
    for (; !done;)
    {
        struct sockaddr_in6 caddr;
        socklen_t caddr_len = sizeof(caddr);
        // Wait for a client to connect
        const int client = accept(server, (struct sockaddr *)&caddr, &caddr_len);
        if (client < 0)
        {
            log_message(distributor_thread_logger, ERROR, DISTRIBUTORTHREAD, "Unable to accept incoming socket");
        }
        else
        {
            // TODO(juan): limitar la cantidad de hilos concurrentes
            struct Connection *c = malloc(sizeof(struct Connection));
            if (c == NULL)
            {
                // We transition into a non-threaded manner
                log_message(distributor_thread_logger, INFO, DISTRIBUTORTHREAD, "Memory Allocation failed, transitioning into non-threaded manner");
                handle_client_without_threading(client, &caddr);
            }
            else
            {
                pthread_t tid;

                c->fd = client;
                c->addr_len = caddr_len;
                buffer_init(&c->info_file_buff, BUFFER_SIZE, c->file_buff);
                buffer_init(&c->info_read_buff, BUFFER_SIZE, c->read_buff);
                buffer_init(&c->info_write_buff, BUFFER_SIZE, c->write_buff);
                memcpy(&(c->addr), &caddr, caddr_len);

                log_message(distributor_thread_logger, INFO, DISTRIBUTORTHREAD, "Attempting to create a Thread");
                if (pthread_create(&tid, 0, handle_connection_pthread, c) != 0)
                {
                    free(c);
                    // We transition into a non-threaded manner
                    log_message(distributor_thread_logger, INFO, DISTRIBUTORTHREAD, "Thread Creation Failed, transitioning into non-threaded manner");
                    handle_client_without_threading(client, &caddr);
                }
            }
        }
    }
    return NULL;
}

// Accomodates resources for the handler
void *handle_connection_pthread(void *args)
{
    struct Connection *c = args;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    char thread_log_filename[30]; // Adjust the size as needed
    sprintf(thread_log_filename, "threadLog%lld.log", milliseconds);
    // Initialize the logger for the client thread
    Logger *client_thread_logs = initialize_logger(thread_log_filename);

    log_message(client_thread_logs, INFO, THREAD, "Executing Thread Routine");

    // Add the Logger to the connection struct
    c->logger = client_thread_logs;

    pthread_detach(pthread_self());
    pop3_handle_connection(c);
    free(args);
    return 0;
}

// Handles each POP3/TCP client
void pop3_handle_connection(struct Connection *conn)
{
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Executing Main Thread Handler");
    struct GlobalConfiguration *g_conf = get_global_configuration();

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Using buffer size : %d", g_conf->buffers_size);

    // Server Buffer Initialization
    // struct buffer serverBuffer;
    // buffer *pServerBuffer = &serverBuffer;
    // // TODO verify this size is the correct, big mails case!
    // uint8_t serverDataBuffer[g_conf->buffers_size]; // This size should be subject to alteration through Configuration Server
    // buffer_init(&serverBuffer, N(serverDataBuffer), serverDataBuffer);

    // // Client Buffer Initialization
    // struct buffer clientBuffer;
    // buffer *pClientBuffer = &clientBuffer;
    // // TODO verify this size is the correct
    // uint8_t clientDataBuffer[g_conf->buffers_size]; // This size should be subject to alteration through Configuration Server
    // buffer_init(&clientBuffer, N(clientDataBuffer), clientDataBuffer);

    // Initial Salute
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Sending Initial Salute");
    send_data("+OK POP3 server ready (^-^)\r\n", &conn->info_write_buff, conn);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;
    int totalBytesRead;

    // Maintain the call and response with the client
    do
    {
        uint8_t *basePointer = buffer_write_ptr(&conn->info_read_buff, &availableSpace);
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
                buffer_write_adv(&conn->info_read_buff, bytesRead);             // Advance buffer
                ptr = buffer_write_ptr(&conn->info_read_buff, &availableSpace); // Update pointer

                char *commandPtr = strstr((char *)basePointer, "\r\n");
                while (commandPtr != NULL)
                {
                    *commandPtr = '\0'; // Null-terminate the command
                    log_message(conn->logger, INFO, THREADMAINHANDLER, "Command sent to parser: '%s'", basePointer);
                    parse_input(basePointer, conn, &conn->info_write_buff);

                    // Move to the next command (if any)
                    basePointer = (uint8_t *)(commandPtr + 2);                      // Move beyond the "\r\n"
                    ptr = buffer_write_ptr(&conn->info_read_buff, &availableSpace); // Update pointer

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
        buffer_reset(&conn->info_read_buff);

    } while (!error && bytesRead > 0);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Freeing Resources");
    close(conn->fd);
}

// Sends data through TCP socket
void send_data(const char *data, buffer *p_buffer, struct Connection *conn /*, bool isMultiLine*/)
{
    size_t data_length = strlen(data);

    send_n_data(data, data_length, p_buffer, conn);
}

// Sends data through TCP socket
void send_n_data(const char *data, size_t length, struct buffer *p_buffer, struct Connection *conn)
{
    if (length > 1024)
    {
        log_message(conn->logger, ERROR, THREADMAINHANDLER, "Data to be sent exceeds the maximum allowed length");
        // Handle the error, return, or exit as needed
        return;
    }

    size_t available_space;
    uint8_t *base_pointer = buffer_write_ptr(p_buffer, &available_space);

    if (length > available_space)
    {
        log_message(conn->logger, ERROR, THREADMAINHANDLER, "Insufficient space in the buffer to write data");
        // Handle the error, return, or exit as needed
        return;
    }

    // Copy the specified length of data to the buffer
    memcpy(base_pointer, data, length);

    // Null-terminate the data to ensure it's a valid string
    base_pointer[length] = '\0';

    // Advance the write pointer in the buffer by the number of bytes written
    buffer_write_adv(p_buffer, length);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "SENDING DATA");

    // Send the data to the client
    sock_blocking_write(conn->fd, p_buffer);

    // Reset buffer pointers for reuse
    buffer_reset(p_buffer);
}

// Function to handle the client without threading
void handle_client_without_threading(int client, const struct sockaddr_in6 *caddr)
{
    // Create a unique log file name for the thread
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    char thread_log_filename[30]; // Adjust the size as needed
    sprintf(thread_log_filename, "threadLog%lld.log", milliseconds);

    // Initialize the logger with the thread-specific log file
    Logger *client_thread_logs = initialize_logger(thread_log_filename);

    log_message(client_thread_logs, ERROR, ITERATIVETHREAD, "Iterative thread running");

    // Create a connection struct with the necessary information
    struct Connection conn;
    conn.fd = client;
    conn.addr_len = sizeof(struct sockaddr_in6);
    memcpy(&(conn.addr), caddr, sizeof(struct sockaddr_in6));
    conn.logger = client_thread_logs; // You can set it to NULL or initialize a separate logger here

    // Call the original handling function
    pop3_handle_connection(&conn);

    // Close the client socket
    close(client);
}

// ------------------------------------------------------------------------------------------------------

// Attends and responds Configuration clients
void *handle_configuration_requests(void *arg)
{
    Logger *configuration_logger = initialize_logger("configurationServer.log");
    log_message(configuration_logger, INFO, CONFIGTHREAD, "Configuration Server is Running...");

    struct GlobalConfiguration *g_conf = get_global_configuration();

    int udp_server = *((int *)arg);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Configuration Client Buffer Initialization
    struct buffer config_client_buffer;
    buffer *p_config_client_buffer = &config_client_buffer;
    uint8_t config_client_data_buffer[g_conf->buffers_size];
    buffer_init(&config_client_buffer, N(config_client_data_buffer), config_client_data_buffer);

    // Configuration Server Buffer Initialization
    struct buffer config_server_buffer;
    buffer *p_config_server_buffer = &config_server_buffer;
    uint8_t config_server_data_buffer[g_conf->buffers_size];
    buffer_init(&config_server_buffer, N(config_server_data_buffer), config_server_data_buffer);

    int received_bytes;
    size_t available_space = 0;

    // Create ClientInfo structure
    struct UDPClientInfo client_info = {
        .udp_server = udp_server,
        .client_addr = &client_addr,
        .client_addr_len = client_addr_len};

    while (1)
    {
        uint8_t *ptr = buffer_write_ptr(p_config_client_buffer, &available_space);

        // Receive data from the client
        received_bytes = recvfrom(udp_server, ptr, available_space, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        log_message(configuration_logger, INFO, CONFIGTHREAD, "Client Request arrived");

        if (received_bytes == -1)
        {
            log_message(configuration_logger, ERROR, CONFIGTHREAD, "Error receiving data from client");
            break;
        }
        else if (received_bytes > 255)
        { // Invalid command
            if (send_data_udp(configuration_logger, &client_info, p_config_client_buffer, "Command is too big") == -1)
            {
                break;
            }
        }

        config_client_data_buffer[received_bytes] = '\0';
        buffer_write_adv(p_config_client_buffer, received_bytes + 1);
        log_message(configuration_logger, INFO, CONFIGTHREAD, "Received Command %s", config_client_data_buffer);
        send_data_udp(configuration_logger, &client_info, p_config_server_buffer, (char *)config_client_data_buffer);
        config_parse_input(configuration_logger, &client_info, p_config_server_buffer, config_client_data_buffer);
        buffer_reset(p_config_client_buffer);
        // parse_config_command(logger, &client_info, pConfigServerBuffer,); // USEN send_data_udp()!

        // EJEMPLO DE USO DE SEND DATA
        // Echo the data back to the client using the managed function
    }

    return NULL;
}

// Function to send data to the client, managing buffer advancement and resetting
int send_data_udp(Logger *logger, const struct UDPClientInfo *client_info, buffer *p_buffer, char *data)
{
    log_message(logger, INFO, CONFIGTHREAD, "Responding client");

    size_t data_length = strlen(data);

    if (data_length > 255)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Data to be sent exceeds the maximum allowed length");
        // Handle the error, return, or exit as needed
        return -1;
    }

    size_t available_space;
    uint8_t *base_pointer = buffer_write_ptr(p_buffer, &available_space);

    if (data_length > available_space)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Insufficient space in the buffer to write data");
        // Handle the error, return, or exit as needed
        return -1;
    }

    // Copy the data to the buffer
    memcpy(base_pointer, data, data_length);

    // Null-terminate the data to ensure it's a valid string
    base_pointer[data_length] = '\0';

    // Advance the write pointer in the buffer by the number of bytes written
    buffer_write_adv(p_buffer, data_length);

    // Send the data to the client
    int sent_bytes = sendto(client_info->udp_server, base_pointer, data_length, 0, (struct sockaddr *)client_info->client_addr, client_info->client_addr_len);
    if (sent_bytes == -1)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Error sending data to client");
    }

    // Reset buffer pointers for reuse
    buffer_reset(p_buffer);

    return sent_bytes;
}
