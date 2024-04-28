#include <stdio.h>
#ifndef PROJ2_H
#define PROJ2_H

#define MAX_LYZARU 20000
#define MAX_ZASTAVEK 10
#define MAX_KAPACITA 100
#define MIN_KAPACITA 10
#define MAX_TL 10000
#define MAX_TB 1000

// Function declarations
int getRandomNumber(int min, int max, int offset);
int CreateBus(int L, int Z, int TB, FILE *file);
void freeResources(int L);
int main(int argc, char *argv[]);

#endif