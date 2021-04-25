#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "utils.h"

int main(void)
{
    int i;
    double lambda = 3.0;
    int iter = 15;
    srand((unsigned)time(NULL));

    printf("=================EXPO=================\n");
    printf("lambda=%f\n", lambda);
    for (i = 0; i < iter; i++)
        printf("%f\n", rand_expo(lambda));

    color("cyan");
    printf("=================POISSON=================\n");
    printf("lambda=%f\n", lambda);
    for (i = 0; i < iter; i++)
        printf("%f\n", rand_poisson(lambda));

    return 0;
}
