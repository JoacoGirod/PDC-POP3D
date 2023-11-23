#include "pop3_functions.h"

void retrieve_emails(const char *user_path, struct Connection *conn)
{
    // Process emails in 'cur' directory
    retrieve_emails_from_directory(user_path, "cur", conn);

    // Process emails in 'new' directory
    retrieve_emails_from_directory(user_path, "new", conn);
}

void retrieve_emails_from_directory(const char *user_path, const char *dir_name, struct Connection *conn)
{
    char dir_path[MAX_PATH_LENGTH];
    snprintf(dir_path, MAX_PATH_LENGTH, "%s/%s", user_path, dir_name);
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
            if (conn->num_emails < MAX_EMAILS)
            {
                struct Mail mail;
                mail.index = get_next_index();
                mail.octets = get_file_size(dir_path, entry->d_name);
                snprintf(mail.filename, MAX_FILENAME_LENGTH, "%s", entry->d_name);
                snprintf(mail.folder, MAIL_FOLDER_LENGTH, "%s", dir_name);
                mail.status = UNCHANGED;

                // Add the mail to the user's mailbox
                conn->mails[conn->num_emails++] = mail;

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

    FILE *file = fopen(file_path, READ_BINARY);
    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        perror("Error setting file position");
        fclose(file);
        return 0;
    }
    long size = ftell(file);
    if (size == -1)
    {
        perror("Error getting file size");
        fclose(file);
        return 0;
    }
    fclose(file);

    return (size_t)size;
}

int delete_file(const char *file_path)
{
    // Check if the file exists before attempting to delete it
    if (access(file_path, F_OK) != 0)
    {
        perror("File doesn't exist");
        return -1; // Failure
    }

    // Attempt to delete the file
    if (remove(file_path) == 0)
    {
        printf("File deleted successfully: %s\n", file_path);
        return 0; // Success
    }
    else
    {
        perror("Error deleting file");
        return -1; // Failure
    }
}

int move_file(const char *source_path, const char *dest_path)
{
    // Ensure the source directory exists
    if (access(source_path, F_OK) != 0)
    {
        perror("Source directory doesn't exist");
        return -1; // Failure
    }

    // Ensure the destination directory exists; create it if not
    if (access(dest_path, F_OK) != 0)
    {
        if (mkdir(dest_path, RD_WR_EXE) != 0)
        {
            perror("Error creating destination directory");
            return -1; // Failure
        }
    }

    // Get the filename from the source path
    const char *filename = strrchr(source_path, '/');
    if (filename == NULL)
    {
        filename = source_path;
    }
    else
    {
        filename++; // Move past the '/'
    }

    // Construct the full destination path
    char full_dest_path[MAX_PATH_LENGTH];
    snprintf(full_dest_path, MAX_PATH_LENGTH, "%s/%s", dest_path, filename);

    // Rename the file (move from source to destination)
    if (rename(source_path, full_dest_path) == 0)
    {
        return 0; // Success
    }
    else
    {
        perror("Error moving file");
        return -1; // Failure
    }
}

int move_file_new_to_cur(const char *base_dir, const char *filename)
{
    // /mailroot/username/Maildir/CuroNew "%s/%s/%s/%s/%s", g_conf->mailroot_folder, conn->username, g_conf->maildir_folder, mail->folder, mail->filename
    char from_path[MAX_PATH_LENGTH];
    // g_conf->mailroot_folder, conn->username, g_conf->maildir_folder,
    snprintf(from_path, sizeof(from_path), "%s/new/%s", base_dir, filename);

    char toPath[MAX_PATH_LENGTH];
    snprintf(toPath, sizeof(toPath), "%s/cur", base_dir);

    return move_file(from_path, toPath);
}
