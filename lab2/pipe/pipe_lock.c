#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <assert.h>

/*
    同步和互斥:
        * 进程锁
        * lockf() 
*/

#define READ_END  0
#define WRITE_END 1
#define BUFFER_SIZE 4001

struct mutexHolder {
    pthread_mutex_t mutex;  // must be here !!!!!!!!
};

pid_t pid1, pid2;  

int main() {
    struct mutexHolder *shared_mutex = NULL;
    pthread_mutexattr_t mutexattr;

    int shared_fd = open("shared.txt", O_CREAT | O_RDWR, 0777);
    (void)ftruncate(shared_fd, sizeof(*shared_mutex));
    shared_mutex = (struct mutexHolder *)mmap(NULL, sizeof(*shared_mutex), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
    close(shared_fd);

    /* Reset */
    memset(shared_mutex, 0, sizeof(*shared_mutex));
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_mutex->mutex, &mutexattr);

    int fd[2]; 
    char InPipe[BUFFER_SIZE];  	         
    const char* c1="1";  
    const char* c2="2";

    pipe(fd);    
                     
    while((pid1 = fork()) == -1);  

    if(pid1 == 0) { /* child process1 */            
        lockf(fd[1],1,0);
        for (int i = 0; i < (BUFFER_SIZE - 1) / 2; ++i)
        {
            // pthread_mutex_lock(&shared_mutex->mutex);
            write(fd[WRITE_END], c1, 1);
            // pthread_mutex_unlock(&shared_mutex->mutex);
        }
        sleep(5);   
        lockf(fd[1],0,0);         
        exit(0);    
    } 
    else {
        while((pid2 = fork()) == -1);          
        if(pid2 == 0) { /* child process2 */  
            lockf(fd[1],1,0);                 
            for (int i = 0; i < (BUFFER_SIZE - 1) / 2; ++i)
            {
                // pthread_mutex_lock(&shared_mutex->mutex);
                write(fd[WRITE_END], c2, 1);
                // pthread_mutex_unlock(&shared_mutex->mutex);
            }
            sleep(5); 
            lockf(fd[1],0,0); 
            exit(0); 
        } 
        else { /* father process */
            wait(NULL);
            // lockf(fd[0],1,0);  
            read(fd[READ_END], InPipe, BUFFER_SIZE - 1);
            // lockf(fd[0],0,0);  
            InPipe[BUFFER_SIZE - 1] = '\0';
            printf("%s\n",InPipe);
            exit(0);                        				
        } 
    }

    pthread_mutex_destroy(&shared_mutex->mutex);

    munmap(shared_mutex, sizeof(*shared_mutex));
    unlink("shared.txt");

    return 0;
} 
