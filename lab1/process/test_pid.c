#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int global = 0;

int main()
{
    pid_t pid, pid1;

    /* fork a child process */
    pid = fork();
    if (pid < 0) {
        /* error occurred */
        fprintf(stderr, "Fork Failed");
        return 1;
    }
    else if (pid == 0) { /* child process */
        pid1 = getpid();
        printf("child: pid = %d\n",pid); /* A */
        printf("child: pid1 = %d\n",pid1); /* B */
        global += 1;
        printf("child: global = 1\n");
        printf("child: global_address = %x\n", &global);
    }
    else { /* parent process */
        pid1 = getpid();
        printf("parent: pid = %d\n",pid); /* C */
        printf("parent: pid1 = %d\n",pid1); /* D */
        global += 2;
        printf("parent: global = 2\n");
        printf("parent: global_address = %x\n", &global);
        wait(NULL);
    }

    global += 1;
    printf("final: global = %d\n", global);

    return 0;
}
