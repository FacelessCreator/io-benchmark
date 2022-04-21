#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

#define MODE_SERIAL 0
#define MODE_RANDOM 1

#define DEFAULT_BLOCK_SIZE 512

static char * file_path = 0;
static int block_size = DEFAULT_BLOCK_SIZE;
static int mode = MODE_SERIAL;
static int help_required = 0;

static struct option opts [] = {
    {"file", required_argument, 0, 'f'},
    {"block-size", required_argument, 0, 'b'},
    {"randomly", no_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char * argv []) {
    // read args
    int opt_c;
    int opt_i;
    while ((opt_c = getopt_long(argc, argv, "f:b:rh", opts, &opt_i)) != -1)
    {
        switch (opt_c)
        {
        case '?':
            // something went wrong while parsing; stop
            return 1;
            break;
        case 'f':
            file_path = optarg;
            break;
        case 'b':
            block_size = atoi(optarg);
            break;
        case 'r':
            mode = MODE_RANDOM;
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
        printf("IO benchmark reader\n");
        printf("--file PATH | -f PATH sets path to file to read (required argument)\n");
        printf("--block-size SIZE | -b SIZE sets block size to read each time (required argument)\n");
        printf("--randomly | -r makes reader to lseek each time to random block\n");
        printf("--help | -h shows this tip\n");
    }
    // check options
    if (!file_path) {
        fprintf(stderr, "File path was not set. See help\n");
        return 2;
    }
    if (block_size <= 0) {
        fprintf(stderr, "Block size was not set properly. See help\n");
        return 3;
    }
    // do reading
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Can't open file %s\n", file_path);
        return 4;
    }
    if (mode == MODE_RANDOM) {
        struct stat fstat;
        if (stat(file_path, &fstat) != 0) {
            fprintf(stderr, "Can't get size of file %s\n", file_path);
            return 5;
        }
        off_t file_size = fstat.st_size;
        // prepare random
        srand(time(0));
        // read randomly
        off_t blocks_count = file_size / block_size;
        off_t random_off;
        ssize_t read_bytes;
        void * buf = malloc(block_size);
        for (long i = 0; i < blocks_count; ++i) {
            random_off = (rand() % blocks_count) * block_size;
            if (blocks_count > RAND_MAX) { // for large files
                lseek(fd, random_off, SEEK_CUR);
            } else {
                lseek(fd, random_off, SEEK_SET);
            }
            read_bytes = read(fd, buf, block_size);
            if (read_bytes == 0) { // end of file
                lseek(fd, 0, SEEK_SET); // start from the beginning
            }
            if (read_bytes == -1) {
                fprintf(stderr, "Error while reading file %s\n", file_path);
                return 6;
            }
        }
        free(buf);
    } else {
        void * buf = malloc(block_size);
        ssize_t read_bytes;
        do
        {
            read_bytes = read(fd, buf, block_size);
        } while (read_bytes == block_size);
        if (read_bytes == -1) {
            fprintf(stderr, "Error while reading file %s\n", file_path);
            return 6;
        }
        free(buf);
    }
    close(fd);
    return 0;
}
