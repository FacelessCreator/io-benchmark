#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define MODE_SERIAL 0
#define MODE_RANDOM 1

#define DEFAULT_BLOCK_SIZE 512
#define DEFAULT_SOURCE_PATH "/dev/urandom"

static char * file_path = 0;
static char * source_path = DEFAULT_SOURCE_PATH;
static int block_size = DEFAULT_BLOCK_SIZE;
static int mode = MODE_SERIAL;
static int help_required = 0;
static long blocks_count = 0;

static struct option opts [] = {
    {"file", required_argument, 0, 'f'},
    {"source", required_argument, 0, 's'},
    {"block-size", required_argument, 0, 'b'},
    {"count", required_argument, 0, 'c'},
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
        case 's':
            source_path = optarg;
            break;
        case 'b':
            block_size = atoi(optarg);
            break;
        case 'c':
            blocks_count = atol(optarg);
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
        printf("IO benchmark writer\n");
        printf("--file PATH | -f PATH sets path to file to write (required argument)\n");
        printf("--source PATH | -s PATH sets the source of bytes. Default value is %s\n", DEFAULT_SOURCE_PATH);
        printf("--block-size SIZE | -b SIZE sets block size to write each time. Default value is %d\n", DEFAULT_BLOCK_SIZE);
        printf("--count COUNT | -c COUNT sets count of blocks to write\n");
        printf("--randomly | -r makes writer to lseek each time to random block\n");
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
    if (blocks_count <= 0) {
        fprintf(stderr, "Blocks count was not set properly. See help\n");
        return 3;
    }
    // do reading
    int fd = open(file_path, O_WRONLY | O_CREAT, 0644);
    int source_fd = open(source_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Can't open file %s\n", file_path);
        return 4;
    }
    if (source_fd == -1) {
        fprintf(stderr, "Can't open source %s\n", source_path);
        return 10;
    }
    if (mode == MODE_RANDOM) {
        // prepare random
        srand(time(0));
        // write randomly
        void * buf = malloc(block_size);
        off_t random_off;
        for (long i = 0; i < blocks_count; ++i) {
            random_off = (rand() % blocks_count) * block_size;
            lseek(fd, random_off, SEEK_SET);
            if (read(source_fd, buf, block_size) != block_size) {
                fprintf(stderr, "Error while reading source %s\n", source_path);
                return 11;
            }
            if (write(fd, buf, block_size) != block_size) {
                fprintf(stderr, "Error while writing file %s\n", file_path);
                return 6;
            }
        }
        free(buf);
    } else {
        void * buf = malloc(block_size);
        for (long i = 0; i < blocks_count; ++i) {
            if (read(source_fd, buf, block_size) != block_size) {
                fprintf(stderr, "Error while reading source %s\n", source_path);
                return 11;
            }
            if (write(fd, buf, block_size) != block_size) {
                fprintf(stderr, "Error while writing file %s\n", file_path);
                return 6;
            }
        }
        free(buf);
    }
    close(fd);
    close(source_fd);
    return 0;
}
