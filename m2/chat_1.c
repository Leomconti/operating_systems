#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>

#define SZ 80

// typedef struct {
//     char *msg;
//     time_t timestamp;
// } message;

// I want to create a pipe, so I can run a chat between two processes
// I will create a pipe, and then fork the process
int main() {
    int fd;
    char *fifo = "/tmp/chatpipe";
    mkfifo(fifo, 0666);

    // Start writing
    // Define buffers for the Chars input / output
    char in[SZ], out[SZ];

    while (1)
    {
        // --- WRITE ---
        fd = open(fifo, O_WRONLY);

        printf("User1: ");
        fgets(in, SZ, stdin);
        printf("\n");

        write(fd, in, strlen(in) + 1);
        close(fd);

        // --- READ ---
        printf("...\n");
        fd = open(fifo, O_RDONLY);

        read(fd, out, sizeof(out));
        close(fd);

        printf("User2: %s\n", out);
    }

    return 0;
}