#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define DEFAULT_FILE_SIZE 512
#define DEFAULT_SOURCE_PATH "/dev/urandom"

static char * folder_path = 0;
static char * source_path = DEFAULT_SOURCE_PATH;
static int file_size = DEFAULT_FILE_SIZE;
static int help_required = 0;
static long files_count = 0;

static struct option opts [] = {
    {"folder", required_argument, 0, 'f'},
    {"source", required_argument, 0, 's'},
    {"file-size", required_argument, 0, 'b'},
    {"count", required_argument, 0, 'c'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char * argv []) {
    // read args
    int opt_c;
    int opt_i;
    while ((opt_c = getopt_long(argc, argv, "f:s:b:c:h", opts, &opt_i)) != -1)
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
        case 's':
            source_path = optarg;
            break;
        case 'b':
            file_size = atoi(optarg);
            break;
        case 'c':
            files_count = atol(optarg);
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
        printf("IO benchmark filebomb writer\n");
        printf("This utility writes lots of small files in folder.\n");
        printf("--folder PATH | -f PATH sets path to folder to write (required argument)\n");
        printf("--source PATH | -s PATH sets the source of bytes. Default value is %s\n", DEFAULT_SOURCE_PATH);
        printf("--file-size SIZE | -b SIZE sets files size. Default value is %d\n", DEFAULT_FILE_SIZE);
        printf("--count COUNT | -c COUNT sets count of files to write\n");
        printf("--help | -h shows this tip\n");
        return 0;
    }
    // check options
    if (!folder_path) {
        fprintf(stderr, "Folder path was not set. See help\n");
        return 2;
    }
    if (file_size <= 0) {
        fprintf(stderr, "File size was not set properly. See help\n");
        return 3;
    }
    if (files_count <= 0) {
        fprintf(stderr, "Files count was not set properly. See help\n");
        return 3;
    }
    // open source
    int source_fd = open(source_path, O_RDONLY);
    if (source_fd == -1) {
        fprintf(stderr, "Can't open source %s\n", source_path);
        return 10;
    }
    // write files
    void * buf = malloc(file_size);
    char file_path [512];
    for (long i = 0; i < files_count; ++i) {
        sprintf(file_path, "%s/%ld.bin", folder_path, i);
        int fd = open(file_path, O_WRONLY | O_CREAT, 0644);
        if (fd == -1) {
            fprintf(stderr, "Can't open file %s\n", file_path);
            return 4;
        }
        if (read(source_fd, buf, file_size) != file_size) {
            fprintf(stderr, "Error while reading source %s\n", source_path);
            return 11;
        }
        if (write(fd, buf, file_size) != file_size) {
            fprintf(stderr, "Error while writing file %s\n", file_path);
            return 6;
        }
        close(fd);
    }
    free(buf);
    return 0;
}
