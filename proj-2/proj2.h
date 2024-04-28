#include <stdio.h>
#ifndef PROJ2_H
#define PROJ2_H

#define MAX_SKIER_COUNT 20000
#define MAX_STOP_COUNT 10
#define MAX_CAPACITY 100
#define MIN_CAPACITY 10
#define MAX_TL 10000
#define MAX_TB 1000

/// @brief Returns random number with added seed offset
/// @param min Inclusive min value
/// @param max Inclusive max value
/// @param offset OffsetForSeed
/// @return Random number
int GetRandomNumber(int min, int max, int offset);

/// @brief Creates process for ski bus with logick
/// @return 0 if it is parent proccess, 1 if it is child proccess
int CreateBus(int L, int Z, int TB, FILE *file);

/// @brief Free resources without maps
/// @param L Number of skiers
void Free(int L);

int main(int argc, char *argv[]);

#endif