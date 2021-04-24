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
int readMessage(int* pointer);

int main(int argc, char* argv[]) {
    
    char bufferName[50];
    strcpy(bufferName, "");
    char producerSemaphoreName[50];
    char consumerSemaphoreName[50];
    char metadataSemaphoreName[50];
    int averageTime = -1;

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
        if (strcmp(argv[i], "-t") == 0) { 
            averageTime = atoi(argv[i + 1]);
        }
    }

    if (strcmp(bufferName, "") == 0) {
        printf("No se pudo determinar el nombre o el largo del buffer\n");
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

    int check = 1;
    int firstTime = 1;
    int totalSize;
    int readIndex;

    while (check == 1) {
        if (sem_wait(semc) < 0) {
            printf("[sem_wait] Failed\n");
            return 1;
        }

        if (sem_wait(semm) < 0) {
            printf("[sem_wait] Failed\n");
            return 1;
        }

        if (firstTime == 1) {
            metadata->consumerActives += 1;
            metadata->consumerTotal += 1;
            totalSize = metadataSize + (128 * metadata->bufferSlots);
            pointer = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            struct Metadata* metadata = (struct Metadata*) pointer;
            firstTime = 0;
        }

        readIndex = metadata->consumerIndex;
        metadata->consumerIndex += 1;

        if (metadata->consumerIndex == metadata->bufferSlots){
            metadata->consumerIndex = 0;
        }

        metadata->currentMessages -= 1;

        if (sem_post(semm) < 0) {
            printf("[sem_post] Failed\n");
            return 1;
        }

        if (readMessage(pointer + (metadataSize * (readIndex + 1))) > 0) {
            check = 0;
            break;
        }

        if (sem_post(semp) < 0) {
            printf("[sem_post] Failed\n");
            return 1;
        }

        sleep(averageTime);
    }

    if (sem_wait(semm) < 0) {
        printf("[sem_wait] Failed\n");
        return 1;
    }

    metadata->consumerActives -= 1;
    //Agregar tiempos

    if (sem_post(semm) < 0) {
        printf("[sem_post] Failed\n");
        return 1;
    }

    printf("Termine\n");

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

int readMessage(int* pointer) {

    char* message = (char*) (pointer);
    printf("Se leyo el mensaje: %s\n", message);

    if (strcmp(message, "STOP") == 0) {
        strcpy(message, "");
        return 1;
    }
    else {
        strcpy(message, "");
        return 0;
    }
}