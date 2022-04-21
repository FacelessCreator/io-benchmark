#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#define WRITER_PATH "build/io-benchmark-writer"
#define READER_PATH "build/io-benchmark-reader"

#define FILE_NAMES_START "io-benchmark-"

#define DEFAULT_BLOCK_SIZE 512
#define DEFAULT_PROCESSES_COUNT 1

static char * folder_path = 0;
static long total_size = 0;
static long block_size = DEFAULT_BLOCK_SIZE;
static int processes_count = DEFAULT_PROCESSES_COUNT;
static int flag_randomly = 0;
static int flag_help = 0;

static struct option opts [] = {
    {"folder", required_argument, 0, 'f'},
    {"size", required_argument, 0, 's'},
    {"block-size", required_argument, 0, 'b'},
    {"processes", required_argument, 0, 'p'},
    {"randomly", no_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int read_args(int argc, char * argv []) {
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
            folder_path = optarg;
            break;
        case 's':
            total_size = atol(optarg);
            break;
        case 'b':
            block_size = atol(optarg);
            break;
        case 'p':
            processes_count = atoi(optarg);
            break;
        case 'r':
            flag_randomly = 1;
            break;
        case 'h':
            flag_help = 1;
            break;
        default:
            break;
        }
    }
    return 0;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void print_args(char * const * args) {
    int i = 0;
    printf("args: ");
    while (args[i] != 0)
    {
        printf("%s ", args[i]);
        ++i;
    }
    printf("\n");
}

int fork_and_exec(const char * path, char * const * args) {
    print_args(args);
    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    } else if (pid == 0) { // child
        char * env [] = {NULL};
        if(execve(path, args, env) == -1) {
            fprintf(stderr, "Execution of %s failed\n", path);
            exit(2);
        }
    }
    return 0;
}

int launch_writer(int id) {
    char args_string [512];
    long blocks_count = total_size / processes_count / block_size;
    sprintf(args_string, WRITER_PATH " --file %s/" FILE_NAMES_START "%d.bin --block-size %ld --count %ld", folder_path, id, block_size, blocks_count); // TODO add randomly flag
    char ** args = str_split(args_string, ' ');
    return fork_and_exec(WRITER_PATH, args);
}

int launch_reader(int id) {
    char args_string [512];
    sprintf(args_string, READER_PATH " --file %s/" FILE_NAMES_START "%d.bin --block-size %ld", folder_path, id, block_size); // TODO add randomly flag
    char ** args = str_split(args_string, ' ');
    return fork_and_exec(READER_PATH, args);
}

clock_t launch_tests(int (* launch_func) (int)) {
    clock_t start_time = clock();
    // launch
    for (int i = 0; i < processes_count; ++i) {
        if (launch_func(i)) {
            fprintf(stderr, "Launch of test %d failed\n", i);
        }
    }
    // wait for writers to finish
    int writer_status;
    for (int i = 0; i < processes_count; ++i) {
        wait(&writer_status);
    }
    // return delta
    return clock() - start_time;
}

void print_help() {
    printf("IO benchmark\n");
    printf("--folder PATH | -f PATH sets folder to create files (required argument)\n");
    printf("--size SIZE | -s SIZE sets total size to write and read in bytes. (required argument)\n");
    printf("--block-size SIZE | -b SIZE sets block size to write and read each time. Default value is %d\n", DEFAULT_BLOCK_SIZE);
    printf("--processes COUNT | -p COUNT sets count of parallel processes\n");
    printf("--randomly | -r makes tests to lseek each time to random block\n");
    printf("--help | -h shows this tip\n");
}

int main(int argc, char * argv []) {
    if (read_args(argc, argv)) {
        return 1; // error already printed
    }
    // check args
    if (flag_help) {
        print_help();
        return 0;
    }
    if (!folder_path) {
        fprintf(stderr, "Folder path was not set. See help\n");
        return 2;
    }
    if (total_size <= 0) {
        fprintf(stderr, "Size was not set. See help\n");
        return 2;
    }
    // do writing tests
    clock_t writing_time = launch_tests(&launch_writer);
    // sync
    clock_t sync_start_time = clock();
    system("sync");
    writing_time += clock() - sync_start_time;
    // report
    double writing_seconds = ((double) writing_time) / CLOCKS_PER_SEC;
    printf("Written in %f s\n", writing_seconds);
    // flush disk cache (root only)
    // TODO
    // do reading tests
    clock_t reading_time = launch_tests(&launch_reader);
    // report
    double reading_seconds = ((double) reading_time) / CLOCKS_PER_SEC;
    printf("Read in %f s\n", reading_seconds);
    // clear
    // TODO
    return 0;
}
