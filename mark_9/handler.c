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

bool isMore(char letter1, char letter2) {
    return letter2 > letter1;
}

// Process handler
void handler(int fifo1, int fifo2) {

    int N;
    read(fifo1, &N, sizeof(N));
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    char result[BUFFER_SIZE];
    int pointer = 0;
    char error_buffer[10] = "Not data\n";

    while((bytes = read(fifo1, buffer, BUFFER_SIZE)) > 0 && (bytes > 0 && N > 0 && N <= bytes)) {

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

        if(pointer != 0) {
            result[++pointer] = '\n';
            write(fifo2, result, pointer + 2);
        } else {
            write(fifo2, error_buffer, 10);
        }
    }

    close(fifo1);
    close(fifo2);
}


int main() {
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    // Open named pipe for reading
    int fd1 = open(fifo1, O_RDONLY);
    if(fd1 == -1) {
        printf("Error opening FIFO for reading\n");
        exit(EXIT_FAILURE);
    }

    // Create and open named pipe for writing
    mkfifo(fifo2, 0666);
    int fd2 = open(fifo2, O_WRONLY);
    if(fd2 == -1) {
        printf("Error opening FIFO for writing\n");
        exit(EXIT_FAILURE);
    }

    // Run handler
    handler(fd1, fd2);

    // Close pipes
    close(fd1);
    close(fd2);
}