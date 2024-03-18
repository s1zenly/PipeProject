#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>


#define BUFFER_SIZE 5000
#define FIFO1 "fifo1"
#define FIFO2 "fifo2"

bool isMore(char letter1, char letter2) {
    return letter2 > letter1;
}

// Process reader
void reader(const char *input_file, int fifo1) {

    // Open input file
    int fd = open(input_file, O_RDONLY);
    if(fd == -1) {
        printf("Error opening input file\n");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    // Send data in handler
    while((bytes = read(fd, buffer, BUFFER_SIZE)) > 0) {
        write(fifo1, buffer, bytes);
    }

    close(fifo1);
    close(fd);
}

// Process handler
void handler(int N, int fifo1, int fifo2) {

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    char result[BUFFER_SIZE];
    int pointer = 0;
    char error_buffer[10] = "Not data\n";

    if((bytes = read(fifo1, buffer, BUFFER_SIZE)) > 0 && (bytes > 0 && N > 0 && N <= bytes)) {

        if(N == 1 && bytes > 0) {
            result[pointer++] = buffer[bytes - 1];
        } else {
            for(int i = bytes - 1; i > 0; --i) {
                for(int j = i; j > 0; --j) {
                    if(pointer == (N - 1) || !isMore(buffer[j], buffer[j - 1])) {
                        break;
                    } else {
                        result[pointer++] = buffer[j];
                        result[pointer] = buffer[j - 1];
                    }
                }

                if(pointer == N - 1) {
                    break;
                }

                pointer = 0;
                memset(result, 0, pointer + 1);
            }
        }
    }

    if(pointer != 0) {
        write(fifo2, result, pointer + 1);
    } else {
        write(fifo2, error_buffer, 10);
    }

    close(fifo1);
    close(fifo2);
}

// Process sender
void sender(const char *output_file, int fifo2) {

    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd == -1) {
        printf("Error opening output file\n");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    while((bytes = read(fifo2, buffer, BUFFER_SIZE)) > 0) {
        write(fd, buffer, bytes);
    }

    close(fifo2);
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Usage: %s <N> <input_file> <output_file>", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Take data with console
    const int N = atoi(argv[1]);
    const char *input_file = argv[2];
    const char *output_file = argv[3];

    // Create two named pipe
    if(mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1) {
        printf("Error creating FIFO\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid;

    // Run reader process
    pid = fork();
    if(pid == -1) {
        printf("Error creating process - reader\n");
        exit(EXIT_FAILURE);
    } else if(pid == 0) {
        reader(input_file, open(FIFO1, O_WRONLY));
        exit(EXIT_SUCCESS);
    }

    // Run handler process
    pid = fork();
    if(pid == -1) {
        printf("Error creating process - handler\n");
        exit(EXIT_FAILURE);
    } else if(pid == 0) {
        handler(N, open(FIFO1, O_RDONLY), open(FIFO2, O_WRONLY));
        exit(EXIT_SUCCESS);
    }

    // Run sender process
    pid = fork();
    if(pid == -1) {
        printf("Error creating process - sender\n");
        exit(EXIT_FAILURE);
    } else if(pid == 0) {
        sender(output_file, open(FIFO2, O_RDONLY));
        exit(EXIT_SUCCESS);
    }

    // Wait child processes
    while(wait(NULL) != -1);

    // Delete named pipes
    unlink(FIFO1);
    unlink(FIFO2);

    return 0;
}