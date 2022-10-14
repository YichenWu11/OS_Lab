#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t mutex;

int sum; /* this data is shared by the thread(s) */

void *func1(void *); /* the thread1 */
void *func2(void *); /* the thread2 */

int main() {
    pthread_mutex_init(&mutex, NULL);

    pthread_t tid1, tid2; /* the thread identifier */
    pthread_attr_t attr1, attr2; /* set of attributes for the thread */

    /* get the default attributes */
    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);

    /* create the thread */
    pthread_create(&tid1, &attr1, func1, NULL);
    pthread_create(&tid2, &attr2, func2, NULL);

    /* now wait for the thread to exit */
    // 第二个参数表示接收到的返回值
    pthread_join(tid1,NULL);
    printf("sum = %d\n",sum);

    pthread_join(tid2,NULL);
    printf("sum = %d\n",sum);

    pthread_mutex_destroy(&mutex);

    return 0;
}

void *func1(void *params) {

    for (int _i = 0; _i < 5000; _i++) {
        // pthread_mutex_lock(&mutex);
        sum++;
        // pthread_mutex_unlock(&mutex);
    }

    pthread_exit(0);
}

void *func2(void *params) {
    for (int _i = 0; _i < 5000; _i++) {
        // pthread_mutex_lock(&mutex);
        sum += 2;
        // pthread_mutex_unlock(&mutex);
    }

    pthread_exit(0);
}
