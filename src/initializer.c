#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
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

    int bufferSize;
    
    int stop;
};

void initializeMedata(struct Metadata* metadata);

int main(int argc, char* argv[]) {

    char* bufferName = "";
    int bufferSize = -1;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            bufferName = argv[i + 1];
        }
        if (strcmp(argv[i], "-l") == 0) { 
            bufferSize = atoi(argv[i + 1]);
        }
    }

    if (bufferSize == -1 || strcmp(bufferName, "") == 0) {
        printf("No se pudo determinar el nombre o el largo del buffer\n");
        return 1;
    }

    printf("Nombre del buffer: %s, Largo de este: %d\n", bufferName, bufferSize);

    int fd = shm_open(bufferName, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("sh,_open()");
        return 1;
    }

    int metadataSize = sizeof(struct Metadata);
    int bufferBytesSize = 128 * bufferSize;
    int totalSize = metadataSize + bufferBytesSize;

    ftruncate(fd, totalSize);

    int* pointer = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct Metadata* metadata = (struct Metadata*) pointer;
    initializeMedata(metadata);
    metadata->bufferSize = bufferSize;

    char* test = (char*) (pointer + metadataSize);
    strcpy(test, "AHHHHHHHHHHH");

    printf("%s\n", test);

    return 0;
}

void initializeMedata(struct Metadata* metadata) {
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
}