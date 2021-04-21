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

int main(int argc, char* argv[]) {

    char* bufferName = "";

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            bufferName = argv[i + 1];
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
    
    int totalSize = metadataSize + (128 * metadata->bufferSize);
    pointer = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    printf("%d\n", metadata->stop);
    metadata->stop = 1;
    printf("%d\n", metadata->stop);

    char* test = (char*) (pointer + metadataSize);
    printf("%s\n", test);

    close(fd);
    shm_unlink(bufferName);   

    return 0;
}