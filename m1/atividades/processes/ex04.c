#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t childpid;

    childpid = fork(); // creates a child process

    if (childpid == -1) {
        perror("Failed to fork");
        return 1;
    }

    printf("Childpid: %d \n", childpid);

    if (childpid == 0) { // Child pid == 0 means it has created the process 
        printf("I'm inside, here's the children code \n");
        printf("The children id: %d \n", getpid());
        execl("/bin/ls", "ls", "-l", NULL); // This calls a new program, in this case ls, which is already compiled and can just be called
        perror("Child failed to exec ls");
        return 1;
    }

    if (childpid != wait(NULL)) { // What the parent runs
        perror("Parent failed to wait due to signal or error");
        return 1;
    }
    return 0;
}
