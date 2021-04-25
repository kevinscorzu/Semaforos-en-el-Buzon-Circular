#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
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
time_t rawtime;

int initializeSemaphores(char *producerSemaphoreName, char *consumerSemaphoreName, char *metadataSemaphoreName);
void writeAutomaticMessage(int *pointer, int index, int producerActives, int consumerActives, char *color);
void writeManualMessage(int *pointer, char *message, int index, int producerActives, int consumerActives, char *color);
void writeStopMessage(int *pointer, int index, int producerActives, int consumerActives, char *color);

int main(int argc, char *argv[])
{

    srand(time(0));
    rawtime = time(NULL);

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
    int writeIndex;
    int consumerActives;
    int producerActives;
    char message[100];
    strcpy(message, "");

    int producedMessages = 0;
    int totalWaitingTime = 0;
    int totalBlockedTime = 0;
    int totalKernelTime = 0;
    int totalUserTime = 0;
    int stop;
    int waitingTime;

    while (check == 1)
    {
        if (manualMode == 1)
        {
            printf("Escriba su Mensaje y Presione Enter:\n");
            fgets(message, 100, stdin);
            message[strcspn(message, "\n")] = 0;
        }

        if (sem_wait(semp) < 0)
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
            metadata->producerActives += 1;
            metadata->producerTotal += 1;
            totalSize = metadataSize + (MessageSize * metadata->bufferSlots);
            pointer = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            struct Metadata *metadata = (struct Metadata *)pointer;
            firstTime = 0;
        }

        if (metadata->stop == 1 && metadata->consumerActives == 0)
        {
            metadata->producerActives -= 1;
            metadata->totalUserTime += totalUserTime;
            metadata->totalKernelTime += totalKernelTime;
            metadata->totalWaitingTime += totalWaitingTime;
            metadata->totalBlockedTime += totalBlockedTime;
            check = 0;
        }

        writeIndex = metadata->producerIndex;
        metadata->producerIndex += 1;

        if (metadata->producerIndex == metadata->bufferSlots)
        {
            metadata->producerIndex = 0;
        }

        consumerActives = metadata->consumerActives;
        producerActives = metadata->producerActives;

        if (check == 1)
        {
            metadata->messageAmount += 1;
            metadata->currentMessages += 1;
            producedMessages += 1;
            stop = metadata->stop;

            if (sem_post(semm) < 0)
            {
                printf("[sem_post] Failed\n");
                return 1;
            }

            if (stop == 1 && consumerActives != 0)
            {
                writeStopMessage(pointer + metadataSize + (MessageSize * writeIndex), writeIndex, producerActives, consumerActives, color);
                if (sem_post(semc) < 0)
                {
                    printf("[sem_post] Failed\n");
                    return 1;
                }

                waitingTime = rand_expo(averageTime);
                sleep(waitingTime);
            }
            else if (manualMode == 0)
            {
                writeAutomaticMessage(pointer + metadataSize + (MessageSize * writeIndex), writeIndex, producerActives, consumerActives, color);
                if (sem_post(semc) < 0)
                {
                    printf("[sem_post] Failed\n");
                    return 1;
                }

                waitingTime = rand_expo(averageTime);
                sleep(waitingTime);
            }
            else
            {
                writeManualMessage(pointer + metadataSize + (MessageSize * writeIndex), message, writeIndex, producerActives, consumerActives, color);
                if (sem_post(semc) < 0)
                {
                    printf("[sem_post] Failed\n");
                    return 1;
                }

                waitingTime = rand_expo(averageTime);
                sleep(waitingTime);
            }
        }
        else
        {
            if (sem_post(semm) < 0)
            {
                printf("[sem_post] Failed\n");
                return 1;
            }

            waitingTime = rand_expo(averageTime);
            sleep(waitingTime);
        }
    }

    setColor("red");
    printf("====================FINALIZADO====================\n");
    setColor(color);
    printf("ID del Productor: %d\n", pid);
    setColor("red");
    printf("Numero de Mensajes Producidos: %d\n", producedMessages);
    printf("Detenido por el Finalizador/Usuario\n");
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

void writeAutomaticMessage(int *pointer, int index, int producerActives, int consumerActives, char *color)
{
    char *message = (char *)(pointer);
    int magicNumber = (rand() % 7);
    struct tm *ptm = localtime(&rawtime);

    sprintf(message, "Numero Magico: %d, PID: %d, Mensaje: Hola, este es el Primer Proyecto de Principios de Sistemas Operativos, Fecha: %02d/%02d/%d, Hora: %02d:%02d:%02d", magicNumber, pid, ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    setColor("green");
    printf("====================INSERTADO====================\n");
    printf("Se inserto el mensaje:\n");
    setColor(color);
    printf("%s\n", message);
    setColor("green");
    printf("En la posicion %d del buzon circular\n", index);
    printf("Cantidad de Productores Vivos al Escribir el Mensaje: %d\n", producerActives);
    printf("Cantidad de Consumidores Vivos al Escribir el Mensaje: %d\n", consumerActives);
    printf("=================================================\n\n");
    setColor("");

    return;
}

void writeManualMessage(int *pointer, char *userInputMessage, int index, int producerActives, int consumerActives, char *color)
{
    char *message = (char *)(pointer);
    int magicNumber = -1;

    if (strcmp(userInputMessage, "Finalizar") != 0)
    {
        magicNumber = (rand() % 7);
    }

    struct tm *ptm = localtime(&rawtime);

    sprintf(message, "Numero Magico: %d, PID: %d, Mensaje: %s, Fecha: %02d/%02d/%d, Hora: %02d:%02d:%02d", magicNumber, pid, userInputMessage, ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    setColor("yellow");
    printf("====================INSERTADO (MANUAL)====================\n");
    printf("Se inserto el mensaje:\n");
    setColor(color);
    printf("%s\n", message);
    setColor("yellow");
    printf("En la posicion %d del buzon circular\n", index);
    printf("Cantidad de Productores Vivos al Escribir el Mensaje: %d\n", producerActives);
    printf("Cantidad de Consumidores Vivos al Escribir el Mensaje: %d\n", consumerActives);
    printf("==========================================================\n\n");
    setColor("");

    return;
}

void writeStopMessage(int *pointer, int index, int producerActives, int consumerActives, char *color)
{
    char *message = (char *)(pointer);
    int magicNumber = -1;
    struct tm *ptm = localtime(&rawtime);

    sprintf(message, "Numero Magico: %d, PID: %d, Mensaje: Finalizar, Fecha: %02d/%02d/%d, Hora: %02d:%02d:%02d", magicNumber, pid, ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    setColor("green");
    printf("====================INSERTADO====================\n");
    printf("Se inserto el mensaje:\n");
    setColor(color);
    printf("%s\n", message);
    setColor("green");
    printf("En la posicion %d del buzon circular\n", index);
    printf("Cantidad de Productores Vivos al Escribir el Mensaje: %d\n", producerActives);
    printf("Cantidad de Consumidores Vivos al Escribir el Mensaje: %d\n", consumerActives);
    printf("=================================================\n\n");
    setColor("");

    return;
}