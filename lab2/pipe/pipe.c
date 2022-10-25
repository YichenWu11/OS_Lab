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

pid_t pid1, pid2;  

int main() {

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
            write(fd[WRITE_END], c1, 1);
        }
        lockf(fd[1],0,0);         
        sleep(2);   
        exit(0);    
    } 
    else {
        while((pid2 = fork()) == -1);          
        if(pid2 == 0) { /* child process2 */  
            lockf(fd[1],1,0);                 
            for (int i = 0; i < (BUFFER_SIZE - 1) / 2; ++i)
            {
                write(fd[WRITE_END], c2, 1);
            }
            lockf(fd[1],0,0); 
            sleep(2); 
            exit(0); 
        } 
        else { /* father process */
            wait(NULL);
            lockf(fd[0],1,0);  
            ssize_t size = read(fd[READ_END], InPipe, BUFFER_SIZE - 1);
            if(size > 0)
                InPipe[size] = '\0';
            else if(size == 0)
                printf("quit\n");
            else
                printf("error\n");
            lockf(fd[0],0,0);  
            printf("%s\n",InPipe);
            exit(0);                        				
        } 
    }

    return 0;
} 
