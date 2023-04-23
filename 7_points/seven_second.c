#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COUNT_CLIENTS 100
#define MAX_COUNT_PAINTS 5
sem_t* paints[MAX_COUNT_PAINTS];
int clients;

int main(int argc, char *argv[]) {
    // Инициализируем семафор
    sem_t* sem_gallery = sem_open("/gallery", O_CREAT | O_EXCL, 0644, MAX_COUNT_CLIENTS);
    paints[0] = sem_open("/first", O_CREAT | O_EXCL, 0644, 11);
    paints[1] = sem_open("/second", O_CREAT | O_EXCL, 0644, 11);
    paints[2] = sem_open("/third", O_CREAT | O_EXCL, 0644, 11);
    paints[3] = sem_open("/fourth", O_CREAT | O_EXCL, 0644, 11);
    paints[4] = sem_open("/fifth", O_CREAT | O_EXCL, 0644, 11);
    int value = 0;
    while (value <= MAX_COUNT_CLIENTS) {
        sem_getvalue(sem_gallery, &value);
        sleep(2);
    }
    // Удаляем и закрываем именовынные семафоры
    sem_unlink("/gallery");
    sem_close(sem_gallery);
    sem_unlink("/first");
    sem_unlink("/second");
    sem_unlink("/third");
    sem_unlink("/fourth");
    sem_unlink("/fifth");
    sem_close(paints[0]);
    sem_close(paints[1]);
    sem_close(paints[2]);
    sem_close(paints[3]);
    sem_close(paints[4]);
    exit(0);
}

