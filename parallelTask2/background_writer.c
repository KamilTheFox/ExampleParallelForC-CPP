#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct thread_data {
    char* filename;
    int write_count;
};

void* background_writer(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    

    
    FILE* file = fopen(data->filename, "w");
    if (file == NULL) {
        printf("Ошибка открытия файла!\n");
        return NULL;
    }
    
    for (int i = 1; i <= data->write_count; i++) {
        fprintf(file, "Запись #%d из отсоединённого потока\n", i);
        fflush(file);
        printf("Фоновый поток: записано %d/%d\n", i, data->write_count);
        sleep(1);
    }
    
    fclose(file);
    printf("Фоновый поток завершил запись и автоматически освободит ресурсы!\n");
    return NULL;
}

int main() {
    pthread_t background_thread;
    struct thread_data data;
    
    data.filename = "output.txt";
    data.write_count = 5;
    
    printf("Основной поток: запускаю ОТСОЕДИНЁННЫЙ поток для фоновой записи...\n");
    
    if (pthread_create(&background_thread, NULL, background_writer, &data) != 0) {
        perror("Ошибка создания потока");
        return 1;
    }
    pthread_detach(background_thread);
    printf("Основной поток: фоновая запись запущена, продолжаю работу...\n");
    
    for (int i = 1; i <= 3; i++) {
        printf("Основной поток: выполняю свою работу #%d\n", i);
    }
    
    printf("Основной поток: завершаю работу.\n");
    pthread_exit(NULL);
}