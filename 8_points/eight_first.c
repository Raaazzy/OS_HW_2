#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COUNT_PAINTS 5
#define MAX_COUNT_CLIENTS 100
int paints[MAX_COUNT_PAINTS];
key_t keys[MAX_COUNT_PAINTS];
struct sembuf paint_buf[MAX_COUNT_PAINTS];

int main(int argc, char *argv[]) {
    char pathname[] = "eight_first.c";
    // Открываем именованный семафор галереи
    int sem_gallery;
    key_t key_gallery = ftok(pathname, 0);
    struct sembuf first_buf;
    if((sem_gallery = semget(key_gallery, 1, 0666 | IPC_CREAT)) < 0){
        printf("Error with creating semaphore\n");
        exit(-1);
    }
    first_buf.sem_num = 0;
    first_buf.sem_op  = MAX_COUNT_CLIENTS;
    first_buf.sem_flg = 0;
    if(semop(sem_gallery, &first_buf, 1) < 0){
        printf("Error with adding num to semaphore\n");
        exit(-1);
    }
    // Открываем семафоры картин
    for (int i = 0; i < MAX_COUNT_PAINTS; ++i) {
        keys[i] = ftok(pathname, i + 1);
        if((paints[i] = semget(keys[i], 1, 0666 | IPC_CREAT)) < 0){
            printf("Error with creating semaphore\n");
            exit(-1);
        }
        paint_buf[i].sem_num = 0;
        paint_buf[i].sem_op  = 11;
        paint_buf[i].sem_flg = 0;
        if(semop(paints[i], &paint_buf[i], 1) < 0){
            printf("Error with adding num to semaphore\n");
            exit(-1);
        }
    }
    int value = 0;
    while (value <= MAX_COUNT_CLIENTS) {
        value = semctl (sem_gallery, 0, GETVAL, 0);
        sleep(2);
    }
    semctl(sem_gallery, 0, IPC_RMID, 0);
    semctl(paints[0], 0, IPC_RMID, 0);
    semctl(paints[1], 0, IPC_RMID, 0);
    semctl(paints[2], 0, IPC_RMID, 0);
    semctl(paints[3], 0, IPC_RMID, 0);
    semctl(paints[4], 0, IPC_RMID, 0);
    exit(0);
}

