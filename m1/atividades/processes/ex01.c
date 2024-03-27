#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t childpid;
    int i, n;
    
    // This checks if there were exactly two arguments in the command line
    // first one is the file name, second one is the actual parameter we want
    if (argc != 2) { 
        fprintf(stderr, "Usage: %s n\n", argv[0]);
        return 1;
    }

    // get the number of processes that will create from the cli arg
    n = atoi(argv[1]); // atoi converts string to integer

    for (i = 1; i < n; i++){
        if ((childpid = fork()) <= 0){ // Fork creates a new process
            printf("I'm a child \n");
            break;
        }
    }

    while(wait(NULL) > 0); // wait for all of your children

    fprintf(stderr, "i:%d process ID:%ld parent ID:%ld child ID:%ld\n",
            i, (long)getpid(), (long)getppid(), (long)childpid);

    return 0;
}
