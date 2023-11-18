#include "pop3_functions.h"

#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 256
#define MAX_EMAILS 64

void retrieve_emails(const char *user_path, struct connection *conn)
{
    // Process emails in 'cur' directory
    retrieve_emails_from_directory(user_path, "cur", conn);

    // Process emails in 'new' directory
    retrieve_emails_from_directory(user_path, "new", conn);
}

void retrieve_emails_from_directory(const char *user_path, const char *dir_name, struct connection *conn)
{
    char dir_path[MAX_PATH_LENGTH];
    snprintf(dir_path, MAX_PATH_LENGTH, "%s/%s", user_path, dir_name);
    printf("Path %s\n", dir_path);
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        { // Check if it's a regular file
            if (conn->numEmails < MAX_EMAILS)
            {
                struct Mail mail;
                mail.index = get_next_index();
                mail.octets = get_file_size(dir_path, entry->d_name);
                snprintf(mail.filename, MAX_FILENAME_LENGTH, "%s", entry->d_name);
                snprintf(mail.folder, 4, "%s", dir_name);
                mail.status = STATUS_NORMAL;

                // Add the mail to the user's mailbox
                conn->mails[conn->numEmails++] = mail;

                // Process the mail or store it as needed
                // (You can add your logic here)

                // Print information about the mail
                printf("Mail Index: %d\n", mail.index);
                printf("Mail Octets: %lu\n", mail.octets);
                printf("Mail Filename: %s\n", mail.filename);
                printf("Mail Folder: %s\n", mail.folder);
                printf("Mail Status: %d\n", mail.status);
            }
            else
            {
                // Handle the case when the mailbox is full
                fprintf(stderr, "Mailbox is full for user\n");
                break;
            }
        }
    }

    closedir(dir);
}

int get_next_index()
{
    static int index = 0;
    return ++index;
}

size_t get_file_size(const char *dir_path, const char *file_name)
{
    char file_path[MAX_PATH_LENGTH];
    snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", dir_path, file_name);

    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fclose(file);

    return size;
}

void microTesting()
{
    char *user_path = "/tmp/Maildir/testuser";

    // Initialize a connection for the user
    struct connection conn;
    conn.numEmails = 0; // Initialize the number of emails to 0

    // Process emails in 'cur' directory for the user
    retrieve_emails_from_directory(user_path, "cur", &conn);

    // Process emails in 'new' directory for the user
    retrieve_emails_from_directory(user_path, "new", &conn);
}
