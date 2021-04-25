#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define Black "\033[0;30m"
#define Red "\033[0;31m"
#define Green "\033[0;32m"
#define Yellow "\033[0;33m"
#define Blue "\033[0;34m"
#define Purple "\033[0;35m"
#define Cyan "\033[0;36m"
#define White "\033[0;37m"
#define Reset "\033[0m"

double rand_expo(double lambda)
{
    double u;
    u = rand() / (RAND_MAX + 1.0);
    return -log(1 - u) / (1 / lambda);
}

double rand_poisson(double lambda)
{
    double u;
    double L = exp(-lambda);
    double k = 0;
    double p = 1;
    while (p > L)
    {
        //printf("[p] %f [L] %f", p, L);
        k++;
        u = rand() / (RAND_MAX + 1.0);
        p *= u;
    }
    return k - 1;
}

void setColor(char *color)
{
    if (strcmp(color, "red") == 0)
        printf(Red);
    else if (strcmp(color, "green") == 0)
        printf(Green);
    else if (strcmp(color, "blue") == 0)
        printf(Blue);
    else if (strcmp(color, "yellow") == 0)
        printf(Yellow);
    else if (strcmp(color, "purple") == 0)
        printf(Purple);
    else if (strcmp(color, "cyan") == 0)
        printf(Cyan);
    else if (strcmp(color, "white") == 0)
        printf(White);
    else
        printf(Reset);
}
