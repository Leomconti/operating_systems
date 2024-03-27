#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main (int argc, char *argv[]) {
    pid_t childpid;
    int i, n;
    pid_t waitreturn;

    if (argc != 2){
        fprintf(stderr, "Usage: %s processes\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);

    for (i = 1; i < n; i++)
        if ((childpid = fork()) != 0)
            break;

    while(childpid != (waitreturn = wait(NULL)))
        if ((waitreturn == -1) && (errno != EINTR))
            break;

    fprintf(stderr, "I am process %ld, my parent is %ld\n",
            (long)getpid(), (long)getppid());
    // getppid get's the parent process id, and getpid get's the process id

    return 0;
}
