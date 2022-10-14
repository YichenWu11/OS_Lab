#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    printf("\nHello World\n");

    pid_t pid;

    pid = getpid();
    printf("hello_world: pid = %d\n\n",pid); /* C */

    return 0;
}
