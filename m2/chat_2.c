#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

#define SZ 80
#define FIFO_R "/tmp/pipew"
#define FIFO_W "/tmp/piper"

// typedef struct {
//     char *msg;
//     time_t timestamp;
// } message;

void *read_from_pipe(void *arg) {
    int fd_r;
    char out[SZ];

    while (1)
    {
        // --- READ ---
        fd_r = open(FIFO_R, O_RDONLY);

        read(fd_r, out, SZ);
        close(fd_r);

        printf("REC: %s\n", out);
    }
}

int main() {
    int fd;
    pthread_t thread;

    mkfifo(FIFO_W, 0666);

    // Start writing
    // Define buffers for the Chars input / output
    char in[SZ], out[SZ];

    printf("Welcome to the chat!\n");
    // Create thread to read from pipe
    pthread_create(&thread, NULL, read_from_pipe, NULL);
    while (1)
    {
        // --- WRITE ---
        fd = open(FIFO_W, O_WRONLY);

        fgets(in, SZ, stdin);
        printf("\n");

        write(fd, in, strlen(in) + 1);
        close(fd);
    }

    return 0;
}