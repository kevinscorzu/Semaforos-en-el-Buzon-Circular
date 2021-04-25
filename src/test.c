#include "utils.h"

int main(void)
{
    int i;
    double lambda = 2;
    int iter = 15;
    srand((unsigned)time(NULL));

    printf("=================EXPO=================\n");
    printf("lambda=%f\n", lambda);
    for (i = 0; i < iter; i++)
        printf("%f\n", rand_expo(lambda));

    setColor("red");
    printf("=================POISSON=================\n");
    setColor("white");
    printf("lambda=%f\n", lambda);
    for (i = 0; i < iter; i++)
        printf("%f\n", rand_poisson(lambda));

    return 0;
}
