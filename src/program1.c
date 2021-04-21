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
    sem_t *sem_id = sem_open(semName, O_CREAT, 0644, 1);
    if (sem_id == SEM_FAILED)
    {
        perror("Parent  : [sem_open] Failed\n");
        return -1;
    }
    int value;
    sem_getvalue(sem_id, &value);
    printf("Parent  : Wait for Child to Print %d\n", value);

    if (sem_wait(sem_id) < 0)
        printf("Parent  : [sem_wait] Failed\n");

    if (sem_wait(sem_id) < 0)
        printf("Parent  : [sem_wait] Failed\n");
    printf("Parent  : Child Printed! \n");
    if (sem_close(sem_id) != 0)
    {
        perror("Parent  : [sem_close] Failed\n");
        return;
    }
    if (sem_unlink(semName) < 0)
    {
        printf("Parent  : [sem_unlink] Failed\n");
        return;
    }
    return 0;
}