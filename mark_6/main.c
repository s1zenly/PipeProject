#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>


#define BUFFER_SIZE 5000

bool isMore(char letter1, char letter2) {
    return letter2 > letter1;
}

// Process reader
void reader(const char *input_file, int pipe1[]) {
    close(pipe1[0]);

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
        write(pipe1[1], buffer, bytes);
    }

    close(pipe1[1]);
    close(fd);
}

// Process handler
void handler(int N, int pipe1[], int pipe2[]) {
    close(pipe1[1]);
    close(pipe2[0]);

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    char result[BUFFER_SIZE];
    int pointer = 0;
    char error_buffer[10] = "Not data\n";

    if((bytes = read(pipe1[0], buffer, BUFFER_SIZE)) > 0 && (bytes > 0 && N > 0 && N <= bytes)) {

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
        write(pipe2[1], result, pointer + 1);
    } else {
        write(pipe2[1], error_buffer, 10);
    }

    close(pipe1[0]);
    close(pipe2[1]);
}

// Process sender
void sender(const char *output_file, int pipe1[], int pipe2[]) {
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[1]);

    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd == -1) {
        printf("Error opening output file\n");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    while((bytes = read(pipe2[0], buffer, BUFFER_SIZE)) > 0) {
        write(fd, buffer, bytes);
    }

    close(pipe2[0]);
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

    // Create two pipe
    int pipe1[2];
    int pipe2[2];

    // Check on error creating processes
    if(pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        printf("Error creating pipe\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid;

    
    pid = fork();
    if(pid == -1) {
        printf("Error creating process - reader\n");
        exit(EXIT_FAILURE);
    } else if(pid == 0) {
        // Reader process
        reader(input_file, pipe1);
    } else {
        pid = fork();
        if(pid == -1) {
            printf("Error creating process - handler\n");
            exit(EXIT_FAILURE);
        } else if(pid == 0) {
            // Handler process
            handler(N, pipe1, pipe2);
            exit(EXIT_SUCCESS);
        } else {
            // Sender process
            sender(output_file, pipe1, pipe2);
            exit(EXIT_SUCCESS);
        }
    }

    // Close pipe on reading and writing
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    // Wait child processes
    while(wait(NULL) != -1);

    return 0;
}