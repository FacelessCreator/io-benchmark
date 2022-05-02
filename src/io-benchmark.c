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
static int flag_no_clear = 0;
static int flag_help = 0;

static struct option opts [] = {
    {"folder", required_argument, 0, 'f'},
    {"size", required_argument, 0, 's'},
    {"block-size", required_argument, 0, 'b'},
    {"processes", required_argument, 0, 'p'},
    {"randomly", no_argument, 0, 'r'},
    {"no-clear", no_argument, 0, 'c'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

long interpret_string_as_bytes_size(char * s) {
    long result = atol(s);
    size_t l = strlen(s);
    switch (s[l-1])
    {
    case 'k': case 'K':
        result *= 1024;
        break;
    case 'm': case 'M':
        result *= 1024*1024;
        break;
    case 'g': case 'G':
        result *= 1024*1024*1024;
        break;
    default:
        break;
    }
    return result;
}

int read_args(int argc, char * argv []) {
    int opt_c;
    int opt_i;
    while ((opt_c = getopt_long(argc, argv, "f:s:b:p:rh", opts, &opt_i)) != -1)
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
            total_size = interpret_string_as_bytes_size(optarg);
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
        case 'c':
            flag_no_clear = 1;
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
    //print_args(args);
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
    sprintf(args_string, WRITER_PATH " --file %s/" FILE_NAMES_START "%d.bin --block-size %ld --count %ld", folder_path, id, block_size, blocks_count);
    if (flag_randomly) {
        strcat(args_string, " --randomly");
    }
    char ** args = str_split(args_string, ' ');
    return fork_and_exec(WRITER_PATH, args);
}

int launch_reader(int id) {
    char args_string [512];
    sprintf(args_string, READER_PATH " --file %s/" FILE_NAMES_START "%d.bin --block-size %ld", folder_path, id, block_size);
    if (flag_randomly) {
        strcat(args_string, " --randomly");
    }
    char ** args = str_split(args_string, ' ');
    return fork_and_exec(READER_PATH, args);
}

double launch_tests(int (* launch_func) (int)) {
    struct timespec start_time;
    timespec_get(&start_time, TIME_UTC);
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
    struct timespec end_time;
    timespec_get(&end_time, TIME_UTC);
    return (end_time.tv_sec-start_time.tv_sec) + 1e-9 * (end_time.tv_nsec-start_time.tv_nsec);
}

double do_sync() {
    struct timespec start_time;
    timespec_get(&start_time, TIME_UTC);
    system("sync");
    struct timespec end_time;
    timespec_get(&end_time, TIME_UTC);
    return (end_time.tv_sec-start_time.tv_sec) + 1e-9 * (end_time.tv_nsec-start_time.tv_nsec);
}

int clear() {
    for (int i = 0; i < processes_count; ++i) {
        char command [512];
        sprintf(command, "rm %s/%s%d.bin", folder_path, FILE_NAMES_START, i);
        system(command);
    }
    return 0;
}

int drop_cache_if_root() {
    if (getuid() == 0) { // if root
        system("echo 1 > /proc/sys/vm/drop_caches");
    }
    return 0;
}

void print_help() {
    printf("IO benchmark\n");
    printf("This utility writes and reads a few large files and records operation time.\n");
    printf("--folder PATH | -f PATH sets folder to create files (required argument)\n");
    printf("--size SIZE | -s SIZE sets total size to write and read in bytes. You can use K (kibibytes), M (mebibytes) and G (gibibytes) ending (required argument)\n");
    printf("--block-size SIZE | -b SIZE sets block size to write and read each time. Default value is %d\n", DEFAULT_BLOCK_SIZE);
    printf("--processes COUNT | -p COUNT sets count of parallel processes\n");
    printf("--randomly | -r makes tests to lseek each time to random block\n");
    printf("--no-clear prevents benchmark from clearing temp files\n");
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
    if (processes_count <= 0) {
        fprintf(stderr, "Process count was not set properly. See help\n");
        return 2;
    }
    if (block_size <= 0) {
        fprintf(stderr, "Block size was not set properly. See help\n");
        return 2;
    }
    // do writing tests
    double writing_time = launch_tests(&launch_writer);
    // sync
    writing_time += do_sync();
    // report
    printf("Written in %f s\n", writing_time);
    // flush disk cache (root only)
    drop_cache_if_root();
    // do reading tests
    double reading_time = launch_tests(&launch_reader);
    // report
    printf("Read in %f s\n", reading_time);
    // clear
    if (!flag_no_clear) {
        if (clear()) {
            return 3; // error already printed
        }
    }
    return 0;
}
