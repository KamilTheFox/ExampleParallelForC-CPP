#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

void cleanup_handler(void *arg) {
    printf("Поток отменён! Выполняю cleanup...\n");
}

void* thread_func(void* arg) {
    pthread_cleanup_push(cleanup_handler, NULL);
    
    printf("Длительная операция началась\n");
    for (int i = 1; i <= 10; i++) {
        printf("Итерация %d из 10\n", i);
        sleep(1);
        //pthread_testcancel();
    }
    
    pthread_cleanup_pop(false);
    printf("Операция завершена нормально\n");
    return NULL;
}

int main() {
    pthread_t thread;
    
    printf("Main: создаю поток для длительной операции\n");
    pthread_create(&thread, NULL, thread_func, NULL);
    
    sleep(3);
    
    printf("Main: отправляю запрос на отмену потока\n");
    //pthread_cancel(thread);
    
    pthread_join(thread, NULL);
    printf("Main: поток завершён\n");
    
    return 0;
}