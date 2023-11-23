#include "server_utils.h"

#define THO 1000
#define OK_POP3_SERVER_READY "+OK POP3 server ready (^-^)\r\n"
#define ERR_HIGH_DEMAND "-ERR Connection refused due to high demand (^-^)\r\n"
#define MAX_LOG_FILE_SHORT 50

static bool done = false;
void sigterm_handler(const int signal)
{
    printf("\nSignal %d, cleaning up and exiting\n", signal); // make the main logger a global variable?
    done = true;
}

// Attends POP3 clients and assigns unique blocking threads to each one
void *serve_pop3_concurrent_blocking(void *server_ptr)
{
    // Transform parameter
    int server = *((int *)server_ptr);

    // Initialize Logger
    char thread_log_file_name[MAX_LOG_FILE_SHORT]; // Adjust the size as needed
    sprintf(thread_log_file_name, "distributor_thread_%llx.log", (unsigned long long)pthread_self());
    Logger *distributor_thread_logger = initialize_logger(thread_log_file_name);

    // Update Statitics
    struct GlobalStatistics *g_stat = get_global_statistics();
    g_stat->total_clients++;

    log_message(distributor_thread_logger, INFO, DISTRIBUTORTHREAD, "Awaiting for connections...");
    while (!done)
    {
        // Accept connections from both IPv4 and IPv6
        struct sockaddr_in6 caddr;
        socklen_t caddr_len = sizeof(caddr);
        const int client = accept(server, (struct sockaddr *)&caddr, &caddr_len);

        if (client < 0)
        {
            log_message(distributor_thread_logger, ERROR, DISTRIBUTORTHREAD, "Unable to accept incoming socket");
        }
        else
        {
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

                if (g_stat->concurrent_clients > MAX_CONCURRENT_USERS)
                {
                    log_message(distributor_thread_logger, INFO, DISTRIBUTORTHREAD, "Unable to accept incoming socket due to high demand");
                    send_data(ERR_HIGH_DEMAND, &c->info_write_buff, c);
                }
                else if (pthread_create(&tid, 0, handle_connection_pthread, c) != 0)
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
    // Transform parameter
    struct Connection *c = args;

    // Initialize logger with uniqueness through time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char thread_log_filename[MAX_LOG_FILE_SHORT];
    sprintf(thread_log_filename, "client_thread_%lld.log", (long long)tv.tv_sec * THO + tv.tv_usec / THO);
    Logger *client_thread_logs = initialize_logger(thread_log_filename);

    // Add the Logger to the connection struct
    c->logger = client_thread_logs;

    log_message(client_thread_logs, INFO, THREAD, "Executing Main POP3 Handler");

    pthread_detach(pthread_self());
    pop3_handle_connection(c);
    free(args);
    return 0;
}

// Handles each POP3/TCP client
void pop3_handle_connection(struct Connection *conn)
{
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Executing Main Thread Handler");

    // Logging Unique Configuration
    struct GlobalConfiguration *g_conf = get_global_configuration();
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Using buffer size : %d", g_conf->buffers_size);

    // Update Global Statistics
    struct GlobalStatistics *g_stat = get_global_statistics();
    g_stat->concurrent_clients++;

    // Initial Salute
    log_message(conn->logger, INFO, THREADMAINHANDLER, "Sending Initial Salute");
    send_data(OK_POP3_SERVER_READY, &conn->info_write_buff, conn);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;
    int totalBytesRead;

    // Maintain the call and response with the client
    do
    {
        log_message(conn->logger, INFO, THREADMAINHANDLER, "Awaiting for Command");

        uint8_t *basePointer = buffer_write_ptr(&conn->info_read_buff, &availableSpace);
        uint8_t *ptr = basePointer;
        totalBytesRead = 0;
        do
        {
            bytesRead = recv(conn->fd, ptr, availableSpace, 0);
            totalBytesRead += bytesRead;

            if (totalBytesRead > MAX_COMMAND_LENGTH)
            {
                log_message(conn->logger, ERROR, THREADMAINHANDLER, "Received command exceeds the maximum allowed length, length : %d", totalBytesRead);
                send_data("-ERR Command exceds buffer size\r\n", &conn->info_write_buff, conn);
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
                while (commandPtr != NULL) // There is a complete command
                {
                    // Null-terminate the command (stepping over the \r)
                    *commandPtr = '\0';

                    // Sending the complete command to the parser
                    log_message(conn->logger, INFO, THREADMAINHANDLER, "Command sent to parser: '%s'", basePointer);
                    parse_input(basePointer, conn);

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
        // we reset the buffer only after the pipelining
        buffer_reset(&conn->info_read_buff);
    } while (!error && bytesRead > 0);

    log_message(conn->logger, INFO, THREADMAINHANDLER, "Closing Connection");

    // Closing pipe
    close(conn->fd);
    // Updating Global Statistics
    g_stat->concurrent_clients--;
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
    memcpy(base_pointer, data, length);
    base_pointer[length] = '\0';
    buffer_write_adv(p_buffer, length);
    sock_blocking_write(conn->fd, p_buffer);
    buffer_reset(p_buffer);
}

// Function to handle the client without threading
void handle_client_without_threading(int client, const struct sockaddr_in6 *caddr)
{
    // Initializing unique log file name
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char thread_log_filename[MAX_LOG_FILE_SHORT];
    sprintf(thread_log_filename, "client_not_threaded_%lld.log", (long long)tv.tv_sec * THO + tv.tv_usec / THO);
    // Initialize the logger with the thread-specific log file
    Logger *client_thread_logs = initialize_logger(thread_log_filename);

    // Create a connection struct with the necessary information
    struct Connection conn;
    conn.fd = client;
    conn.addr_len = sizeof(struct sockaddr_in6);
    memcpy(&(conn.addr), caddr, sizeof(struct sockaddr_in6));
    conn.logger = client_thread_logs;

    log_message(client_thread_logs, ERROR, ITERATIVETHREAD, "Executing Main POP3 Handler");

    pop3_handle_connection(&conn);
    // Close connection but no resources to free
    close(client);
}

// ------------------------------------------------------------------------------------------------------

// Attends and responds Configuration clients
void *handle_configuration_requests(void *arg)
{
    // Transform argument
    int udp_server = *((int *)arg);

    // Initialize log
    char thread_log_file_name[MAX_LOG_FILE_SHORT];
    sprintf(thread_log_file_name, "config_server_%llx.log", (unsigned long long)pthread_self());
    Logger *configuration_logger = initialize_logger(thread_log_file_name);

    log_message(configuration_logger, INFO, CONFIGTHREAD, "Configuration Server Running...");

    struct ConfigServer *s_conf = get_config_server();
    buffer_init(&s_conf->info_write_buff, BUFFER_SIZE, s_conf->write_buff);
    buffer_init(&s_conf->info_read_buff, BUFFER_SIZE, s_conf->read_buff);

    int received_bytes;
    size_t available_space = 0;

    // Create ClientInfo structure
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct UDPClientInfo client_info = {
        .udp_server = udp_server,
        .client_addr = &client_addr,
        .client_addr_len = client_addr_len};

    while (!done)
    {
        uint8_t *ptr = buffer_write_ptr(&s_conf->info_read_buff, &available_space);

        received_bytes = recvfrom(udp_server, ptr, available_space, 0, (struct sockaddr *)&client_addr, &client_addr_len);

        log_message(configuration_logger, INFO, CONFIGTHREAD, "Client Request arrived");

        if (received_bytes == -1)
        {
            log_message(configuration_logger, ERROR, CONFIGTHREAD, "Error receiving data from client");
            break;
        }
        else if (received_bytes > MAX_COMMAND_LENGTH)
        {
            send_data_udp(configuration_logger, &client_info, "-ERR Command is too big\r\n");
            break;
        }

        s_conf->read_buff[received_bytes] = '\0';
        buffer_write_adv(&s_conf->info_read_buff, received_bytes + 1);

        log_message(configuration_logger, INFO, CONFIGTHREAD, "Received Command %s", s_conf->read_buff);

        // PARSE!
        config_parse_input(configuration_logger, &client_info, s_conf->read_buff);

        buffer_reset(&s_conf->info_read_buff);
    }
    return NULL;
}

// Function to send data to the client, managing buffer advancement and resetting
int send_data_udp(Logger *logger, const struct UDPClientInfo *client_info, char *data)
{
    struct ConfigServer *s_conf = get_config_server();
    buffer *p_buffer = &s_conf->info_write_buff;

    log_message(logger, INFO, CONFIGTHREAD, "Responding client");
    size_t data_length = strlen(data);
    if (data_length > MAX_COMMAND_LENGTH)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Data to be sent exceeds the maximum allowed length");
        return -1;
    }

    size_t available_space;
    uint8_t *base_pointer = buffer_write_ptr(p_buffer, &available_space);
    if (data_length > available_space)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Insufficient space in the buffer to write data");
        return -1;
    }

    memcpy(base_pointer, data, data_length);
    base_pointer[data_length] = '\0';
    buffer_write_adv(p_buffer, data_length);
    int sent_bytes = sendto(client_info->udp_server, base_pointer, data_length, 0, (struct sockaddr *)client_info->client_addr, client_info->client_addr_len);
    if (sent_bytes == -1)
    {
        log_message(logger, ERROR, CONFIGTHREAD, "Error sending data to client");
        return -1;
    }
    buffer_reset(p_buffer);
    return sent_bytes;
}
