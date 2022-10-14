#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>

void *func(void *);

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
        // exec
        pthread_t tid;
        pthread_attr_t attr;

        pthread_attr_init(&attr);

        pthread_create(&tid, &attr, func, NULL);

        pthread_join(tid,NULL);

        pid1 = getpid();
        printf("child: pid = %d\n",pid); /* A */
        printf("child: pid1 = %d\n",pid1); /* B */
    }
    else { /* parent process */
        pid1 = getpid();
        printf("parent: pid = %d\n",pid); /* C */
        printf("parent: pid1 = %d\n",pid1); /* D */
        wait(NULL);
    }

    return 0;
}

void *func(void *params) {
    pthread_t tid = pthread_self();
    printf("Inner thread: tid = %ld\n", tid);
    // execlp("../bin/hello_world.exe", NULL);
    system("../bin/hello_world.exe");

    pthread_exit(0);
}
