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
int clients;
pid_t pid;

int main(int argc, char *argv[]) {
    sleep(1);
    if (argc != 3) {
        printf("Error arguments\n");
        return 1;
    }
    FILE *in_file = fopen(argv[1], "r");
    FILE *out_file = fopen(argv[2], "w");
    if (!in_file || !out_file) {
        printf("Error opening files\n");
        return 1;
    }
    fscanf(in_file, "%d", &clients);
    printf("Number of clients: %d\nNumber of paints: %d\n", clients, MAX_COUNT_PAINTS);
    char pathname[] = "eight_first.c";
    // Именованный семафор галереи
    int sem_gallery;
    key_t key_gallery = ftok(pathname, 0);
    struct sembuf first_buf;
    if((sem_gallery = semget(key_gallery, 1, 0666)) < 0){
        perror("Error creating parent process\n");
        exit(-1);
    }
    // Семафоры картин
    for (int i = 0; i < MAX_COUNT_PAINTS; ++i) {
        keys[i] = ftok(pathname, i + 1);
        if((paints[i] = semget(keys[i], 1, 0666)) < 0){
            printf("Error creating child process\n");
            exit(-1);
        }
    }
    for (int i = 1; i <= clients; ++i) {
        pid = fork();
        // Дочерний процесс
        if (pid < 0) {
            perror("Error fork");
            semctl(sem_gallery, 0, IPC_RMID, 0);
            exit(-1);
            // Родительский процесс
        } else if (pid == 0) {
            // посетителя впускает вахтер
            first_buf.sem_num = 0;
            first_buf.sem_op  = -1;
            first_buf.sem_flg = 0;
            if(semop(sem_gallery, &first_buf, 1) < 0) {
                printf("Error\n");
                exit(-1);
            }
            printf("Client %d come in \n", i);
            fprintf(out_file, "Client %d come in \n", i);
            srand(getpid());
            int watched[MAX_COUNT_PAINTS] = {0, 0, 0, 0, 0};
            for (int i = 0; i < MAX_COUNT_PAINTS; ++i) {
                if (watched[0] == 1 && watched[1] == 1 && watched[2] == 1 && watched[3] == 1 && watched[4] == 1) {
                    break;
                }
                int j = rand() % MAX_COUNT_PAINTS;
                while (watched[j]) {
                    j = rand() % MAX_COUNT_PAINTS;
                }
                watched[j] = 1;
                paint_buf[j].sem_num = 0;
                paint_buf[j].sem_op  = -1;
                paint_buf[j].sem_flg = 0;
                if(semop(paints[j], &paint_buf[j], 1) < 0){
                    printf("Error\n");
                    exit(-1);
                }
                printf("New client %d is watching paint number %d\n", i, j);
                fprintf(out_file, "New client %d is watching paint number %d\n", i, j);
                sleep(rand() % MAX_COUNT_PAINTS);
                printf("Client %d finished watching the paint number %d\n", i, j);
                fprintf(out_file, "Client %d finished watching the paint number %d\n", i, j);
                paint_buf[j].sem_num = 0;
                paint_buf[j].sem_op  = 1;
                paint_buf[j].sem_flg = 0;
                if(semop(paints[j], &paint_buf[j], 1) < 0){
                    printf("Error with adding num to semaphore\n");
                    exit(-1);
                }
            }
            // Освобождаем семафор
            printf("Client %d left\n", i);
            fprintf(out_file, "Client %d left\n", i);
            first_buf.sem_num = 0;
            first_buf.sem_op  = 1;
            first_buf.sem_flg = 0;
            if(semop(sem_gallery, &first_buf, 1) < 0){
                printf("Error with adding num to semaphore\n");
                exit(-1);
            }
            exit(0);
        }
    }
    // Родительский процесс ожидает заверешния работы всех процессов
    if (pid > 0) {
        while (pid = waitpid(-1, NULL, 0)) {
            if (errno == ECHILD) {
                break;
            }
        }
        first_buf.sem_num = 0;
        first_buf.sem_op  = 1;
        first_buf.sem_flg = 0;
        if(semop(sem_gallery, &first_buf, 1) < 0){
            printf("Error with adding num to semaphore\n");
            exit(-1);
        }
        exit(0);
    }
}

