#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

struct Metadata {
    int producerActives;
    int producerTotal;
    int producerIndex;

    int consumerActives;
    int consumerTotal;
    int consumerIndex;
    int consumerTotalDeletedByKey;

    int messageAmount;
    int currentMessages;

    int totalWaitingTime;
    int totalBlockedTime;
    int totalUserTime;
    int totalKernelTime;

    int bufferSlots;

    int stop;
};

sem_t* semp;
sem_t* semc;
sem_t* semm;

int initializeSemaphores(char* producerSemaphoreName, char* consumerSemaphoreName, char* metadataSemaphoreName);
int closeSemaphores(char* producerSemaphoreName, char* consumerSemaphoreName, char* metadataSemaphoreName);
void printData(struct Metadata* metadata);

int main(int argc, char* argv[]) {

    char bufferName[50];
    strcpy(bufferName, "");
    char producerSemaphoreName[50];
    char consumerSemaphoreName[50];
    char metadataSemaphoreName[50];

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(bufferName, argv[i + 1]);
            strcpy(producerSemaphoreName, bufferName);
            strcat(producerSemaphoreName, "p");
            strcpy(consumerSemaphoreName, bufferName);
            strcat(consumerSemaphoreName, "c");
            strcpy(metadataSemaphoreName, bufferName);
            strcat(metadataSemaphoreName, "m");
        }
    }

    if (strcmp(bufferName, "") == 0) {
        printf("No se pudo determinar el nombre del buffer\n");
        return 1;
    }

    printf("Nombre del buffer: %s\n", bufferName);

    int fd = shm_open(bufferName, O_RDWR, 0600);
    if (fd < 0) {
        perror("sh,_open()");
        return 1;
    }

    int metadataSize = sizeof(struct Metadata);

    int* pointer = mmap(NULL, metadataSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct Metadata* metadata = (struct Metadata*) pointer;

    if (initializeSemaphores(producerSemaphoreName, consumerSemaphoreName, metadataSemaphoreName) > 0) {
        printf("No se pudo obtener los datos de los semaforos");
        return 1;
    }

    if (sem_wait(semm) < 0) {
        printf("[sem_wait] Failed\n");
        return 1;
    }

    metadata->stop = 1;

    if (sem_post(semm) < 0) {
        printf("[sem_post] Failed\n");
        return 1;
    }

    int check = 1;

    while (check == 1) {
        if (sem_wait(semm) < 0) {
            printf("[sem_wait] Failed\n");
            return 1;
        }

        if (metadata->producerActives > 0 && metadata->consumerActives == 0) {
            if (sem_post(semp) < 0) {
                printf("[sem_post] Failed\n");
                return 1;
            }
        }

        if (metadata->consumerActives == 0 && metadata->producerActives == 0) {
            check = 0;
        }

        if (sem_post(semm) < 0) {
            printf("[sem_post] Failed\n");
            return 1;
        }
    }

    printData(metadata);

    if (closeSemaphores(producerSemaphoreName, consumerSemaphoreName, metadataSemaphoreName) > 0) {
        printf("No se pudo cerrar los semaforos");
        return 1;
    }

    close(fd);
    shm_unlink(bufferName);   

    return 0;
}

int initializeSemaphores(char* producerSemaphoreName, char* consumerSemaphoreName, char* metadataSemaphoreName) {

    semp = sem_open(producerSemaphoreName, 0);

    if (semp == SEM_FAILED) {
        perror("[sem_open] Failed\n");
        return 1;
    }

    semc = sem_open(consumerSemaphoreName, 0);

    if (semc == SEM_FAILED) {
        perror("[sem_open] Failed\n");
        return 1;
    }

    semm = sem_open(metadataSemaphoreName, 0);

    if (semm == SEM_FAILED) {
        perror("[sem_open] Failed\n");
        return 1;
    }

    return 0;
}

int closeSemaphores(char* producerSemaphoreName, char* consumerSemaphoreName, char* metadataSemaphoreName) {

    if (sem_close(semp) != 0) {
        perror("[sem_close] Failed\n");
        return 1;
    }

    if (sem_unlink(producerSemaphoreName) < 0) {
        printf("[sem_unlink] Failed\n");
        return 1;
    }

    if (sem_close(semc) != 0) {
        perror("[sem_close] Failed\n");
        return 1;
    }

    if (sem_unlink(consumerSemaphoreName) < 0) {
        printf("[sem_unlink] Failed\n");
        return 1;
    }

    if (sem_close(semm) != 0) {
        perror("[sem_close] Failed\n");
        return 1;
    }

    if (sem_unlink(metadataSemaphoreName) < 0) {
        printf("[sem_unlink] Failed\n");
        return 1;
    }

    return 0;
}

void printData(struct Metadata* metadata) {

    printf("Mensajes Totales: %d\n", metadata->messageAmount);
    printf("Mensajes en el Buffer: %d\n", metadata->currentMessages);
    printf("Total de Productores: %d\n", metadata->producerTotal);
    printf("Total de Consumidores: %d\n", metadata->consumerTotal);
    printf("Total de Consumidores Borrados por Llave: %d\n", metadata->consumerTotalDeletedByKey);
    printf("Tiempo Esperando Total: %d\n", metadata->totalWaitingTime);
    printf("Tiempo Bloqueado Total: %d\n", metadata->totalBlockedTime);
    printf("Tiempo de Usuario Total: %d\n", metadata->totalUserTime);
    printf("Tiempo de Kernel Total: %d\n", metadata->totalKernelTime);

    return;
}