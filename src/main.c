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

#include "buffer.h"
#include "netutils.h"
#include "tests.h"

static bool done = false;
static void sigterm_handler(const int signal) {
    printf("signal %d , cleaning up and exiting\n", signal);
    done = true;
}

/**
 * Structure used to transport information from the distributor thread onto the client handler threads
 */
struct connection {
    int fd;
    socklen_t addrlen;
    struct sockaddr_in6 addr;
};


/**
 * Main Handling Function for incoming requests
 */
static void pop3_handle_connection(const int fd, const struct sockaddr *caddr) {
    // Buffer Initialization
    struct buffer serverBuffer;
    buffer *pServerBuffer = &serverBuffer;
    // TODO verify this size is the correct, big mails case!
    uint8_t serverDataBuffer[512];
    buffer_init(&serverBuffer, N(serverDataBuffer), serverDataBuffer);

    // Initial State: Salute
    memcpy(serverDataBuffer, "+OK POP3 server ready (^-^)\r\n", 30);
    buffer_write_adv(pServerBuffer, 30);
    sock_blocking_write(fd, pServerBuffer);

    bool error = false;
    size_t availableSpace;
    ssize_t bytesRead;

    do {
        // Function buffer_write_ptr() returns the pointer to the writable area and the amount of bytes I can write
        uint8_t *ptr = buffer_write_ptr(pServerBuffer, &availableSpace);

        // Receive the information sent by the client and save it in the buffer
        // TODO add relevant flags to this call
        bytesRead = recv(fd, ptr, availableSpace, 0);

        // TODO this could fail if the TCP Stream doesnt have the complete command, it should have a cumulative variable and reset it when a command is succesfully parsed
        if (bytesRead > 0) {
            // Check if the received command exceeds the maximum length defined in the RFC Extension
            if (bytesRead > 255) {
                // Handle the error (you may want to implement proper error handling)
                fprintf(stderr, "Received command exceeds the maximum allowed length\n");
                error = true;
                break;
            }

            // Advance the write pointer in the buffer by the number of bytes received
            buffer_write_adv(pServerBuffer, bytesRead);

            // Process the received data (you may want to implement your command handling logic here)
            // For now, let's print what we received
            printf("Received from client: %.*s", (int)bytesRead, ptr);
        } else if (bytesRead == 0) {
            // Connection closed by the client
            break;
        } else {
            // Handle the error (you may want to implement proper error handling)
            perror("recv");
            error = true;
            break;
        }
    } while (!error && bytesRead > 0);

    close(fd);
}



/**
 * Unique thread handler
 */
static void *handle_connection_pthread(void *args) {
    // Session Struct could be initialized here or inside the thread
    // Initializing it here would make it easier to free the resoureces
    // Session Attributes : state, username, directory...

    const struct connection *c = args;
    pthread_detach(pthread_self()); // Configure thread to instantly free resources after its termination
    pop3_handle_connection(c->fd, (struct sockaddr *)&c->addr);
    free(args);
    return 0;
}


/**
 * Attends clients and assigns unique blocking threads to each one
 */
int serve_pop3_concurrent_blocking(const int server) {
    for (;!done;) {
        struct sockaddr_in6 caddr;
        socklen_t caddrlen = sizeof (caddr);
        // Wait for a client to connect
        const int client = accept(server, (struct sockaddr*)&caddr, &caddrlen);
        if (client < 0) {
            perror("unable to accept incoming socket");
        } else {
            // TODO(juan): limitar la cantidad de hilos concurrentes
            struct connection* c = malloc(sizeof (struct connection));
            if (c == NULL) {
                // We transition into an iterative manner
                pop3_handle_connection(client, (struct sockaddr*)&caddr);
            } else {
                pthread_t tid;
                c->fd = client;
                c->addrlen = caddrlen;
                memcpy(&(c->addr), &caddr, caddrlen);
                if (pthread_create(&tid, 0, handle_connection_pthread, c)) {
                    free(c);
                    // We transition into an iterative manner
                    pop3_handle_connection(client, (struct sockaddr*)&caddr);
                }
            }
        }
    }
    return 0;
}

int main(const int argc, const char **argv) {
    unsigned port = 1110; // Standard Port defined in class

    // Port can be changed through arguments
    if(argc == 1) {
        // utilizamos el default
    } else if(argc == 2) {
        char *end     = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1]|| '\0' != *end || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) || sl < 0 || sl > USHRT_MAX) {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
        }
        port = sl;
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);

    const char *err_msg;

    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server < 0) {
        err_msg = "unable to create socket";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d\n", port);

    // Server is configured to not wait for the last bytes transfered after the FIN in the TCP Connection
    // This allows for quick restart of the server, should only be used in development
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(server, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        err_msg = "unable to bind socket";
        goto finally;
    }

    // TODO : change this number from 20 to 500 to allow for more concurrent connections
    if (listen(server, 20) < 0) {
        err_msg = "unable to listen";
        goto finally;
    }

    // Registering SIGTERM helps tools like Valgrind
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);


    err_msg = 0;
    int ret = serve_pop3_concurrent_blocking(server);

finally:
    if(err_msg) {
        perror(err_msg);
        ret = 1;
    }
    if(server >= 0) {
        close(server);
    }
    return ret;
}
