#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>


#define BUFFER_SIZE 256

// Process reader
void reader(int N, int input_fd, int fifo1_fd) {

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    write(fifo1_fd, &N, sizeof(N));

    // Send data in handler
    while((bytes = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        write(fifo1_fd, buffer, bytes);
    }

    close(fifo1_fd);
    close(input_fd);
}

// Process sender
void sender(int output_fd, int fifo2_fd) {

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    while((bytes = read(fifo2_fd, buffer, BUFFER_SIZE)) > 0) {
        write(output_fd, buffer, bytes);
    }

    close(fifo2_fd);
    close(output_fd);
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
    // Named pipes
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    // Open file for reading
    int input_fd = open(input_file, O_RDONLY);
    if(input_fd == -1) {
        printf("Error opening input file\n");
        exit(EXIT_FAILURE);
    }

    // Create and open FIFO1
    mkfifo(fifo1, 0666);
    int fifo1_fd = open(fifo1, O_WRONLY);
    if(fifo1_fd == -1) {
        printf("Error opening FIFO1 for writing\n");
        exit(EXIT_FAILURE);
    }

    // Reading info
    reader(N, input_fd, fifo1_fd);

    // Open output file
    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0064);
    if(output_fd == -1) {
        printf("Error opening output file\n");
        exit(EXIT_FAILURE);
    }

    // Open FIFO2
    int fifo2_fd = open(fifo2, O_RDONLY);
    if(fifo2_fd == -1) {
        printf("Error opening FIFO2 for reading\n");
        exit(EXIT_FAILURE);
    }

    // Writing info
    sender(output_fd, fifo2_fd);

    // Delete FIFOs
    unlink(fifo1);
    unlink(fifo2);

    return 0;
}   