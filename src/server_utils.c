#include "server_utils.h"

// Main Thread Handling Function
void pop3_handle_connection(struct Connection *conn)
{
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Executing Main Thread Handler");
    struct GlobalConfiguration *g_conf = get_global_configuration();

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Using buffer size : %d", g_conf->buffers_size);

    // Server Buffer
    struct buffer serverBuffer;
    buffer *pServerBuffer = &serverBuffer;
    // TODO verify this size is the correct, big mails case!
    uint8_t serverDataBuffer[g_conf->buffers_size]; // This size should be subject to alteration through Configuration Server
    buffer_init(&serverBuffer, N(serverDataBuffer), serverDataBuffer);

    // Client Buffer Initialization
    struct buffer clientBuffer;
    buffer *pClientBuffer = &clientBuffer;
    // TODO verify this size is the correct
    uint8_t clientDataBuffer[g_conf->buffers_size]; // This size should be subject to alteration through Configuration Server
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
void send_data(const char *data, buffer *p_buffer, struct Connection *conn /*, bool isMultiLine*/)
{
    size_t data_length = strlen(data);

    send_n_data(data, data_length, p_buffer, conn);
}

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

// Modify the function call to pass the connection struct
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
        log_message(configuration_logger, DEBUG, CONFIGTHREAD, "BUFFER WRITE PTR %p", ptr);

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
    for (; 1;)
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

// -------------------
// non blocking stuff
// -------------------

// Function to write data to a buffer
void write_to_buffer(const char *data, buffer *p_buffer, struct Connection *conn)
{
    size_t data_length = strlen(data);

    // Ensure that the buffer has enough space
    size_t available_space;
    uint8_t *base_pointer = buffer_write_ptr(p_buffer, &available_space);

    // fprintf(stdout, "pointer %p\n", base_pointer);

    if (data_length > available_space)
    {
        log_message(conn->logger, ERROR, THREADMAINHANDLER, "Insufficient space in the buffer to write data");
        // Handle the error, return, or exit as needed
        return;
    }

    // Copy the specified length of data to the buffer
    memcpy(base_pointer, data, data_length);

    // Null-terminate the data to ensure it's a valid string
    base_pointer[data_length] = '\0';

    // Advance the write pointer in the buffer by the number of bytes written
    buffer_write_adv(p_buffer, data_length);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Writing data to buffer");
}

static const struct fd_handler handler = {
    .handle_read = pop3_read, // shold exist
    .handle_write = pop3_write,
    .handle_block = NULL,
    .handle_close = NULL // should exist
};

void pop3_passive_accept(struct selector_key *key)
{
    struct sockaddr_in6 caddr;
    socklen_t caddr_len = sizeof(caddr);
    // Wait for a client to connect
    const int client = accept(key->fd, (struct sockaddr *)&caddr, &caddr_len);

    if (client == -1)
    {
        perror("Error creating active socket");
    }

    if (selector_fd_set_nio(client) == -1)
    {
        perror("Unable to set FD as non blocking");
    }

    struct Connection *c = calloc(1, sizeof(struct Connection));

    if (c == NULL)
    {
        perror("Error allocating memory for Connection");
        return;
    }

    c->fd = client;
    c->addr_len = caddr_len;
    buffer_init(&c->info_file_buff, BUFFER_SIZE, c->file_buff);
    buffer_init(&c->info_read_buff, BUFFER_SIZE, c->read_buff);
    buffer_init(&c->info_write_buff, BUFFER_SIZE, c->write_buff);
    memcpy(&(c->addr), &caddr, caddr_len);

    // crear buffers
    // dejarlos en el connection
    write_to_buffer("+OK POP3 server ready (^-^)\r\n", &c->info_write_buff, c);

    if (selector_register(key->s, client, &handler, OP_WRITE, c) != SELECTOR_SUCCESS)
    {
        perror("Unable to register FD as non blocking");
    }
}

void pop3_write(struct selector_key *key)
{
    printf("----> Write activated\n");

    struct GlobalStatistics *g_stats = get_global_statistics();
    struct Connection *conn = (struct Connection *)key->data;
    size_t available_space = 0;
    uint8_t *ptr = buffer_read_ptr(&conn->info_write_buff, &available_space);
    ssize_t sent_count = send(key->fd, ptr, available_space, MSG_NOSIGNAL);

    if (sent_count == -1)
    {
        return;
    }
    g_stats->bytes_transfered += sent_count;
    buffer_read_adv(&conn->info_write_buff, sent_count);
    if (buffer_can_read(&conn->info_write_buff))
    {
        selector_set_interest(key->s, key->fd, OP_WRITE);
    }
    selector_set_interest(key->s, key->fd, OP_READ);
    if (selector_set_interest(key->s, key->fd, OP_READ) != SELECTOR_SUCCESS)
    {
        return;
    }
}

void pop3_read(struct selector_key *key)
{
    // printf("----> Read activated\n");
    struct Connection *conn = (struct Connection *)key->data;

    // write_to_buffer("Fuck bitches get Money", &conn->info_write_buff, conn);

    // Guardamos lo que leemos del socket en el buffer de entrada
    // size_t available_space = 0;
    // uint8_t *ptr = buffer_write_ptr(&conn->info_read_buff, &available_space);
    // ssize_t bytes_read = recv(key->fd, ptr, available_space, 0);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;
    int totalBytesRead;

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
            ptr[bytesRead] = '\0';
            printf("Read from socket '%s'\n", (char *)ptr);

            if (totalBytesRead > MAX_COMMAND_LENGTH)
            {
                // log_message(conn->logger, ERROR, THREADMAINHANDLER, "Received command exceeds the maximum allowed length");
                error = true;
                break;
            }

            if (bytesRead > 0) // Some bytes read
            {
                ptr[bytesRead] = '\0'; // Null termination
                // log_message(conn->logger, INFO, THREADMAINHANDLER, "Streamed by TCP: '%s'", ptr);
                buffer_write_adv(&conn->info_read_buff, bytesRead);             // Advance buffer
                ptr = buffer_write_ptr(&conn->info_read_buff, &availableSpace); // Update pointer

                char *commandPtr = strstr((char *)basePointer, "\r\n");
                while (commandPtr != NULL)
                {
                    *commandPtr = '\0'; // Null-terminate the command
                                        // log_message(conn->logger, INFO, THREADMAINHANDLER, "Command sent to parser: '%s'", basePointer);

                    // write_to_buffer((char *)basePointer, &conn->info_write_buff, conn);
                    //  selector_set_interest(key->s, key->fd, OP_WRITE);
                    printf("Passing to parser %s\n", basePointer);

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
                // log_message(conn->logger, INFO, THREADMAINHANDLER, "Connection closed by the client");
                break;
            }
            else // Function failed
            {
                // log_message(conn->logger, ERROR, THREADMAINHANDLER, "Function recv failed");
                error = true;
                break;
            }
        } while (!error); // Continue reading until a complete command is processed

        write_to_buffer("Cambio y fuera\n", &conn->info_write_buff, conn);

        if (selector_set_interest(key->s, key->fd, OP_WRITE) != SELECTOR_SUCCESS)
        {
            return;
        }
        // log_message(conn->logger, INFO, THREADMAINHANDLER, "Resetting client buffer");
        buffer_reset(&conn->info_read_buff);

    } while (!error && bytesRead > 0);
}
