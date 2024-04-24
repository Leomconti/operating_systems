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
    int fd2;
    char *fifo = "/tmp/chatpipe";
    mkfifo(fifo, 0666);

    // Start writing
    // Define buffers for the Chars input / output
    char in[SZ], out[SZ];

    while (1)
    {
        // --- READ ---
        fd2 = open(fifo, O_RDONLY);

        printf("...\n");
        read(fd2, out, SZ);
        close(fd2);

        printf("User1: %s\n", out);

        // --- WRITE ---
        fd2 = open(fifo, O_WRONLY);

        printf("User2: ");
        fgets(in, SZ, stdin);
        printf("\n");

        write(fd2, in, strlen(in)+1);
        close(fd2);
    }

    return 0;
}