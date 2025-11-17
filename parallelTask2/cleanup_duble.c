#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

void cleanup1(void *arg) { 
    printf("🟢 Cleanup 1 из потока %ld\n", (long)arg); 
}

void cleanup2(void *arg) { 
    printf("🔵 Cleanup 2 из потока %ld\n", (long)arg); 
}

void* worker1(void* arg) {
    long thread_id = (long)arg;
    printf("Поток %ld: добавляю cleanup1\n", thread_id);
    pthread_cleanup_push(cleanup1, (void*)thread_id);
    
    sleep(1);  
    
    printf("Поток %ld: вызываю pop(1)\n", thread_id);
    pthread_cleanup_pop(true);  
    
    return NULL;
}

void* worker2(void* arg) {
    long thread_id = (long)arg;
    printf("Поток %ld: добавляю cleanup2\n", thread_id);
    pthread_cleanup_push(cleanup2, (void*)thread_id);
    
    sleep(2);  // Работает дольше
    
    printf("Поток %ld: вызываю pop(0)\n", thread_id);
    pthread_cleanup_pop(true);  
    
    return NULL;
}

int main() {
    pthread_t t1, t2;
    
    pthread_create(&t1, NULL, worker1, (void*)1);
    pthread_create(&t2, NULL, worker2, (void*)2);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    return 0;
}