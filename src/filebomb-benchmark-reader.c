#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>

#define BLOCK_SIZE 512

static char * folder_path = 0;
static int help_required = 0;

static struct option opts [] = {
    {"folder", required_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char * argv []) {
    // read args
    int opt_c;
    int opt_i;
    while ((opt_c = getopt_long(argc, argv, "f:h", opts, &opt_i)) != -1)
    {
        switch (opt_c)
        {
        case '?':
            // something went wrong while parsing; stop
            return 1;
            break;
        case 'f':
            folder_path = optarg;
            break;
        case 'h':
            help_required = 1;
            break;
        default:
            break;
        }
    }
    // check help
    if (help_required) {
        printf("IO benchmark filebomb reader\n");
        printf("This utility reads lots of small files in folder.\n");
        printf("--folder PATH | -f PATH sets path to folder to write (required argument)\n");
        printf("--help | -h shows this tip\n");
        return 0;
    }
    // check options
    if (!folder_path) {
        fprintf(stderr, "Folder path was not set. See help\n");
        return 2;
    }
    // vars
    DIR* dir_fd;
    struct dirent* in_file;
    int fd;
    void * buf = malloc(BLOCK_SIZE);
    ssize_t read_bytes;
    char file_path [512];
    // scanning directory
    dir_fd = opendir(folder_path);
    if (dir_fd == NULL) {
        fprintf(stderr, "Can't open folder %s\n", folder_path);
        return 3;
    }
    // reading files
    while ((in_file = readdir(dir_fd))) {
        if (!strcmp (in_file->d_name, "."))
            continue;
        if (!strcmp (in_file->d_name, ".."))    
            continue;
        sprintf(file_path, "%s/%s", folder_path, in_file->d_name);
        fd = open(file_path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Can't open file %s\n", file_path);
            continue;
        }
        do
        {
            read_bytes = read(fd, buf, BLOCK_SIZE);
        } while (read_bytes == BLOCK_SIZE);
        if (read_bytes == -1) {
            fprintf(stderr, "Error while reading file %s\n", file_path);
            return 6;
        }
        close(fd);
    }
    free(buf);
    closedir(dir_fd);
    return 0;
}
