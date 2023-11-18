// pop3_functions.h

#ifndef POP3_FUNCTION_H
#define POP3_FUNCTION_H
#define _GNU_SOURCE

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "server_utils.h"

void retrieve_emails(const char *user_path, struct connection *conn);
void retrieve_emails_from_directory(const char *user_path, const char *dir_name, struct connection *conn);
int get_next_index();
size_t get_file_size(const char *dir_path, const char *file_name);
void microTesting();
#endif // POP3_FUNCTION_H
