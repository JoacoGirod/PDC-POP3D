// pop3_functions.h

#ifndef POP3_FUNCTION_H
#define POP3_FUNCTION_H
#define _GNU_SOURCE // Highly suspicious

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "server_utils.h"

void retrieve_emails(const char *user_path, struct Connection *conn);
void retrieve_emails_from_directory(const char *user_path, const char *dir_name, struct Connection *conn);
int get_next_index();
size_t get_file_size(const char *dir_path, const char *file_name);

int move_file_new_to_cur(const char *base_dir, const char *filename);
int move_file(const char *user_path, const char *filename);
int delete_file(const char *file_path);

void microTesting();
#endif // POP3_FUNCTION_H
