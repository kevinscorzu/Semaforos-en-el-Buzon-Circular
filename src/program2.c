#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

const char *semName = "asdfsd";

int main()
{
    sem_t *sem_id = sem_open(semName, 0);
    if (sem_id == SEM_FAILED)
    {
        perror("[Promgram2] [sem_open] Failed\n");
        return -1;
    }

    printf("[Promgram2]  I am done! Release Semaphore\n");
    if (sem_post(sem_id) < 0)
        printf("[Promgram2]  [sem_post] Failed \n");

    return 0;
}