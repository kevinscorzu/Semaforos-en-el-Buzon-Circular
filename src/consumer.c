#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include "utils.h"

#define MessageSize 256

struct Metadata
{
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

sem_t *semp;
sem_t *semc;
sem_t *semm;
int pid;

int initializeSemaphores(char *producerSemaphoreName, char *consumerSemaphoreName, char *metadataSemaphoreName);
int readAutomaticMessage(int *pointer, int index, int producerActives, int consumerActives, char *color);

int main(int argc, char *argv[])
{

    char bufferName[50];
    strcpy(bufferName, "");
    char producerSemaphoreName[50];
    char consumerSemaphoreName[50];
    char metadataSemaphoreName[50];
    int averageTime = -1;
    int manualMode = 0;
    pid = getpid();
    char color[50];
    strcpy(color, "white");

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0)
        {
            strcpy(bufferName, argv[i + 1]);
            strcpy(producerSemaphoreName, bufferName);
            strcat(producerSemaphoreName, "p");
            strcpy(consumerSemaphoreName, bufferName);
            strcat(consumerSemaphoreName, "c");
            strcpy(metadataSemaphoreName, bufferName);
            strcat(metadataSemaphoreName, "m");
        }
        if (strcmp(argv[i], "-t") == 0)
        {
            averageTime = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-m") == 0)
        {
            manualMode = 1;
        }
        if (strcmp(argv[i], "-c") == 0)
        {
            strcpy(color, argv[i + 1]);
        }
    }

    if (strcmp(bufferName, "") == 0 || averageTime == -1)
    {
        printf("No se pudo determinar el nombre del buffer o el tiempo de espera promedio\n");
        return 1;
    }

    printf("Nombre del buffer: %s\n", bufferName);

    int fd = shm_open(bufferName, O_RDWR, 0600);
    if (fd < 0)
    {
        perror("sh,_open()");
        return 1;
    }

    int metadataSize = sizeof(struct Metadata);

    int *pointer = mmap(NULL, metadataSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct Metadata *metadata = (struct Metadata *)pointer;

    if (initializeSemaphores(producerSemaphoreName, consumerSemaphoreName, metadataSemaphoreName) > 0)
    {
        printf("No se pudo obtener los datos de los semaforos");
        return 1;
    }

    int check = 1;
    int firstTime = 1;
    int totalSize;
    int readIndex;
    int consumerActives;
    int producerActives;
    int exitCause;
    char message[100];
    strcpy(message, "");

    int consumedMessages = 0;
    int totalWaitingTime = 0;
    int totalBlockedTime = 0;
    int totalKernelTime = 0;
    int totalUserTime = 0;
    int waitingTime;

    while (check == 1)
    {
        if (manualMode == 1)
        {
            printf("Presione Enter para Consumir un Mensaje:\n");
            fgets(message, 100, stdin);
        }

        if (sem_wait(semc) < 0)
        {
            printf("[sem_wait] Failed\n");
            return 1;
        }

        if (sem_wait(semm) < 0)
        {
            printf("[sem_wait] Failed\n");
            return 1;
        }

        if (firstTime == 1)
        {
            metadata->consumerActives += 1;
            metadata->consumerTotal += 1;
            totalSize = metadataSize + (MessageSize * metadata->bufferSlots);
            pointer = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            struct Metadata *metadata = (struct Metadata *)pointer;
            firstTime = 0;
        }

        readIndex = metadata->consumerIndex;
        metadata->consumerIndex += 1;

        if (metadata->consumerIndex == metadata->bufferSlots)
        {
            metadata->consumerIndex = 0;
        }

        metadata->currentMessages -= 1;
        consumerActives = metadata->consumerActives;
        producerActives = metadata->producerActives;
        consumedMessages += 1;

        if (sem_post(semm) < 0)
        {
            printf("[sem_post] Failed\n");
            return 1;
        }

        exitCause = readAutomaticMessage(pointer + metadataSize + (MessageSize * readIndex), readIndex, producerActives, consumerActives, color);

        if (exitCause > 0)
        {
            check = 0;
        }

        if (sem_post(semp) < 0)
        {
            printf("[sem_post] Failed\n");
            return 1;
        }

        waitingTime = rand_poisson(averageTime);
        sleep(waitingTime);
    }

    if (sem_wait(semm) < 0)
    {
        printf("[sem_wait] Failed\n");
        return 1;
    }

    metadata->consumerActives -= 1;
    metadata->totalUserTime += totalUserTime;
    metadata->totalKernelTime += totalKernelTime;
    metadata->totalWaitingTime += totalWaitingTime;
    metadata->totalBlockedTime += totalBlockedTime;

    if (exitCause == 2)
    {
        metadata->consumerTotalDeletedByKey += 1;
    }

    if (sem_post(semm) < 0)
    {
        printf("[sem_post] Failed\n");
        return 1;
    }

    setColor("red");
    printf("====================FINALIZADO====================\n");
    setColor(color);
    printf("ID del Consumidor: %d\n", pid);
    setColor("red");
    printf("Numero de Mensajes Consumidos: %d\n", consumedMessages);

    if (exitCause == 1)
    {
        printf("Detenido por el Finalizador/Usuario\n");
    }
    if (exitCause == 2)
    {
        printf("Detenido por el Numero Magico\n");
    }

    printf("Tiempo Esperando Total: %d\n", totalWaitingTime);
    printf("Tiempo Bloqueado Total: %d\n", totalBlockedTime);
    printf("Tiempo de Usuario Total: %d\n", totalUserTime);
    printf("Tiempo de Kernel Total: %d\n", totalKernelTime);
    setColor("red");
    printf("==================================================\n\n");
    setColor("");

    return 0;
}

int initializeSemaphores(char *producerSemaphoreName, char *consumerSemaphoreName, char *metadataSemaphoreName)
{

    semp = sem_open(producerSemaphoreName, 0);

    if (semp == SEM_FAILED)
    {
        perror("[sem_open] Failed\n");
        return 1;
    }

    semc = sem_open(consumerSemaphoreName, 0);

    if (semc == SEM_FAILED)
    {
        perror("[sem_open] Failed\n");
        return 1;
    }

    semm = sem_open(metadataSemaphoreName, 0);

    if (semm == SEM_FAILED)
    {
        perror("[sem_open] Failed\n");
        return 1;
    }

    return 0;
}

int readAutomaticMessage(int *pointer, int index, int producerActives, int consumerActives, char *color)
{

    char *message = (char *)(pointer);

    setColor("green");
    printf("======================LEIDO======================\n");
    printf("Se leyo el mensaje\n");
    setColor(color);
    printf("%s\n", message);
    setColor("green");
    printf("En la posicion %d del buzon circular\n", index);
    printf("Cantidad de Productores Vivos al Leer el Mensaje: %d\n", producerActives);
    printf("Cantidad de Consumidores Vivos al Leer el Mensaje: %d\n", consumerActives);
    printf("=================================================\n\n");
    setColor("");

    int magicNumber = atoi(&message[15]);
    strcpy(message, "");

    if (magicNumber == -1)
    {
        return 1;
    }

    if (pid % 6 == magicNumber)
    {
        return 2;
    }

    return 0;
}