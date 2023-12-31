// global_config.h

#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"
#include <semaphore.h>

#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 256
#define MAX_EMAILS 64
#define MAX_USERS 10
#define INITIAL_BUFFER_SIZE 2048
#define INITIAL_LOG_FOLDER_NAME "logs"
#define INITIAL_MAILROOT_LOCATION "/tmp"
#define INITIAL_MAILDIRDIR_LOCATION "Maildir"
#define INITIAL_AUTHORIZATION_TOKEN "adminadmin"
#define MAX_FOLDERNAME_LENGTH 256
#define MAX_TRANSFORMATION_SCRIPT_LENGTH 10000
#define AUTHTOKEN_LENGTH 10

struct Users
{
    char *name;
    char *pass;
    sem_t semaphore; // Semaphore for each user
};

typedef struct
{
    char *logFileName;
} Logger;

struct GlobalConfiguration
{
    char *pop3_addr;
    int pop3_port;
    char *conf_addr;
    int conf_port;
    Logger *user_access_log;
    struct Users users[MAX_USERS];
    size_t numUsers;
    size_t buffers_size;
    char logs_folder[MAX_FILENAME_LENGTH];
    char mailroot_folder[MAX_FOLDERNAME_LENGTH];
    char maildir_folder[MAX_FOLDERNAME_LENGTH];
    char authorization_token[AUTHTOKEN_LENGTH + 1];
    bool transformation;
    char transformation_script[MAX_TRANSFORMATION_SCRIPT_LENGTH];
};

struct GlobalStatistics
{
    size_t concurrent_clients;
    size_t total_clients;
    size_t bytes_transfered;
};

struct ConfigServer
{
    uint8_t read_buff[INITIAL_BUFFER_SIZE];
    uint8_t write_buff[INITIAL_BUFFER_SIZE];
    buffer info_write_buff;
    buffer info_read_buff;
};

struct GlobalConfiguration *get_global_configuration();
struct GlobalStatistics *get_global_statistics();
struct ConfigServer *get_config_server();

#endif // GLOBAL_CONFIG_H
