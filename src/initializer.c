#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

#define MessageSize 256

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

void initializeMetadata(struct Metadata* metadata);
int createSemaphores(char* producerSemaphoreName, char* consumerSemaphoreName, char* metadataSemaphoreName, int bufferSlots);

int main(int argc, char* argv[]) {

    char bufferName[50];
    strcpy(bufferName, "");
    char producerSemaphoreName[50];
    char consumerSemaphoreName[50];
    char metadataSemaphoreName[50];
    int bufferSlots = -1;

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
        if (strcmp(argv[i], "-l") == 0) { 
            bufferSlots = atoi(argv[i + 1]);
        }
    }

    if (bufferSlots == -1 || strcmp(bufferName, "") == 0) {
        printf("No se pudo determinar el nombre o el largo del buffer\n");
        return 1;
    }

    printf("Nombre del buffer: %s, Largo de este: %d\n", bufferName, bufferSlots);

    int fd = shm_open(bufferName, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        shm_unlink(bufferName); 
        perror("sh,_open()");
        return 1;
    }

    int metadataSize = sizeof(struct Metadata);
    int bufferSize = MessageSize * bufferSlots;
    int totalSize = metadataSize + bufferSize;

    ftruncate(fd, totalSize);

    int* pointer = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct Metadata* metadata = (struct Metadata*) pointer;
    initializeMetadata(metadata);
    metadata->bufferSlots = bufferSlots;

    if (createSemaphores(producerSemaphoreName, consumerSemaphoreName, metadataSemaphoreName, bufferSlots) > 0) {
        printf("No se pudo abrir los semaforos\n");
        close(fd);
        shm_unlink(bufferName); 
        return 1;
    }

    printf("Buffer y semaforos creados exitosamente\n");

    return 0;
}

void initializeMetadata(struct Metadata* metadata) {

    metadata->producerActives = 0;
    metadata->producerTotal = 0;
    metadata->producerIndex = 0;

    metadata->consumerActives = 0;
    metadata->consumerTotal = 0;
    metadata->consumerIndex = 0;
    metadata->consumerTotalDeletedByKey = 0;

    metadata->messageAmount = 0;
    metadata->currentMessages = 0;

    metadata->totalWaitingTime = 0;
    metadata->totalBlockedTime = 0;
    metadata->totalUserTime = 0;
    metadata->totalKernelTime = 0;

    metadata->stop = 0;

    return;
}

int createSemaphores(char* producerSemaphoreName, char* consumerSemaphoreName, char* metadataSemaphoreName, int bufferSlots) {

    sem_t* semp = sem_open(producerSemaphoreName, O_CREAT, 0644, bufferSlots);

    if (semp == SEM_FAILED)
    {
        perror("[sem_open] Failed\n");
        return 1;
    }

    //sem_init(semp, 1, bufferSlots);

    sem_t* semc = sem_open(consumerSemaphoreName, O_CREAT, 0644, 0);

    if (semc == SEM_FAILED)
    {
        perror("[sem_open] Failed\n");
        return 1;
    }

    //sem_init(semc, 1, 0);


    sem_t* semm = sem_open(metadataSemaphoreName, O_CREAT, 0644, 1);

    if (semm == SEM_FAILED)
    {
        perror("[sem_open] Failed\n");
        return 1;
    }

    //sem_init(semm, 1, 1);

    //printf("%s\n", producerSemaphoreName);
    //printf("%s\n", consumerSemaphoreName);
    //printf("%s\n", metadataSemaphoreName);

    return 0;
}