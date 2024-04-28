#define _GNU_SOURCE
#include "./proj2.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdbool.h>

pid_t *skibusPId;

int *currentLogNumber;
int *currentSkibusStopNumber;
int *currentBusPassangersCount;
int *deliveredSkiersCount;
bool *finished = false; // used for ending skier proccesses

sem_t *logSemaphor;      // used for accesing currentLogNumber and for logging
sem_t *boardSemaphor;    // used for limiting number of skiers stop checks
sem_t **skiersSemaphors; // used for ensuring that every skier checks if bus is in his stop

/// @brief Returns random number with added seed offset
/// @param min Inclusive min value
/// @param max Inclusive max value
/// @param offset OffsetForSeed
/// @return Random number
int GetRandomNumber(int min, int max, int offset)
{
    srand(time(NULL) + offset);
    int final = rand() % (max - min + 1) + min;
    return final;
}

/// @brief Creates process for ski bus with logic
/// @return returns 0 if it is parent proccess, 1 if it is child proccess
int CreateBus(int L, int Z, int TB, FILE *file)
{
    *skibusPId = fork();

    if (*skibusPId == -1)
    {
        fprintf(stderr, "Error occured while creating new child proccess\n");
        return 1;
    }
    if (*skibusPId == 0)
    {
        // First log of ski bus
        sem_wait(logSemaphor);
        fprintf(file, "%d: BUS: started\n", *currentLogNumber);
        fflush(file);
        sem_post(logSemaphor);

        while (*deliveredSkiersCount != L)
        {
            // Added one more stop for final stop
            for (int i = 0; i < Z + 1; i++)
            {
                usleep(GetRandomNumber(0, TB, i));
                sem_wait(logSemaphor);

                (*currentLogNumber)++;
                if (i != Z)
                {
                    fprintf(file, "%d: BUS: arrived to %d\n", *currentLogNumber, i + 1);
                    fflush(file);
                }
                else
                {
                    fprintf(file, "%d: BUS: arrived to final\n", *currentLogNumber);
                    fflush(file);
                }
                fflush(file);
                sem_post(logSemaphor);

                *currentSkibusStopNumber = i + 1;

                // Add number of skiers for stop checks
                for (int i = 0; i < L; i++)
                {
                    sem_post(boardSemaphor);
                }

                // Start skiers stop check sequencion
                sem_post(skiersSemaphors[0]);

                // Wait for finish of skiers stop checks
                sem_wait(skiersSemaphors[L]);

                if (i != Z)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    fprintf(file, "%d: BUS: leaving %d\n", *currentLogNumber, i + 1);
                    fflush(file);
                    sem_post(logSemaphor);
                }
                else
                {
                    fprintf(file, "%d: BUS: leaving final\n", *currentLogNumber);
                    fflush(file);
                }
            }
        }
        sem_wait(logSemaphor);
        (*currentLogNumber)++;
        fprintf(file, "%d: BUS: finish\n", *currentLogNumber);
        fflush(file);
        sem_post(logSemaphor);
        return 1;
    }
    return 0;
}

void Free(int L)
{
    munmap(currentLogNumber, sizeof(int));
    munmap(currentBusPassangersCount, sizeof(int));
    munmap(deliveredSkiersCount, sizeof(int));
    munmap(finished, sizeof(bool));
    munmap(currentSkibusStopNumber, sizeof(int));
    munmap(logSemaphor, sizeof(sem_t));
    munmap(boardSemaphor, sizeof(sem_t));
    for (int i = 0; i <= L; i++)
    {
        munmap(skiersSemaphors[i], sizeof(sem_t));
    }
    free(skibusPId);
}

int main(int argc, char *argv[])
{
    FILE *file = fopen("proj2.out", "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error occured while writing file\n");
        return 1;
    }

    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s L Z K TL TB \n", argv[0]);
        return 1;
    }

    int L = atoi(argv[1]);
    int Z = atoi(argv[2]);
    int K = atoi(argv[3]);
    int TL = atoi(argv[4]);
    int TB = atoi(argv[5]);

    if (L <= 0 ||
        L >= MAX_SKIER_COUNT ||
        TL > MAX_TL ||
        Z <= 0 ||
        Z > MAX_STOP_COUNT ||
        TB < 0 ||
        TL < 0 ||
        K < MIN_CAPACITY ||
        K > MAX_CAPACITY ||
        TB > MAX_TB)
    {
        fprintf(stderr, "Wrong args\n");
        return 1;
    }

    currentLogNumber = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    currentSkibusStopNumber = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    finished = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    currentBusPassangersCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    deliveredSkiersCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    logSemaphor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    boardSemaphor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    skiersSemaphors = mmap(NULL, sizeof(sem_t *) * (L + 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    if ((currentLogNumber == MAP_FAILED) ||
        (currentSkibusStopNumber == MAP_FAILED) ||
        (finished == MAP_FAILED) ||
        (currentBusPassangersCount == MAP_FAILED) ||
        (deliveredSkiersCount == MAP_FAILED) ||
        (logSemaphor == MAP_FAILED) ||
        (boardSemaphor == MAP_FAILED) ||
        (skiersSemaphors == MAP_FAILED))
    {
        fprintf(stderr, "Mapping of shared memory failed\n");
        Free(L);
        return 1;
    }
    *currentLogNumber = 1;

    skibusPId = malloc(sizeof(pid_t));

    sem_init(boardSemaphor, 1, 0);
    sem_init(logSemaphor, 1, 1);

    for (int i = 0; i < L + 1; i++)
    {
        skiersSemaphors[i] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
        sem_init(skiersSemaphors[i], 1, 0);
    }

    if (CreateBus(L, Z, TB, file))
    {
        Free(L);
        fclose(file);
        return 0;
    }

    for (int i = 0; i < L; i++)
    {
        int currentSkiPid = fork();
        int currentSkiIndex = i + 1;
        int targetStop = GetRandomNumber(1, Z, currentSkiIndex);
        int semaphorIndex = i;
        bool boarded = false;
        bool left = false;

        // Check if proccess is child
        if (currentSkiPid == 0)
        {
            sem_wait(logSemaphor);
            (*currentLogNumber)++;
            fprintf(file, "%d: L %d: started \n", *currentLogNumber, currentSkiIndex);
            fflush(file);
            sem_post(logSemaphor);

            usleep(GetRandomNumber(0, TL, currentSkiIndex));
            sem_wait(logSemaphor);
            (*currentLogNumber)++;
            fprintf(file, "%d: L %d: arrived to %d \n", *currentLogNumber, currentSkiIndex, targetStop);
            fflush(file);
            sem_post(logSemaphor);

            while (!*finished || !left)
            {
                sem_wait(skiersSemaphors[semaphorIndex]);
                sem_wait(boardSemaphor);

                fflush(file);
                if (targetStop == *currentSkibusStopNumber && !boarded && *currentBusPassangersCount < K)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    fprintf(file, "%d: L %d: boarding \n", *currentLogNumber, currentSkiIndex);
                    fflush(file);
                    (*currentBusPassangersCount)++;
                    boarded = true;
                    sem_post(logSemaphor);
                }
                if (boarded && *currentSkibusStopNumber == Z + 1 && !left)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    (*currentBusPassangersCount)--;
                    fprintf(file, "%d: L %d: going to ski \n", *currentLogNumber, currentSkiIndex);
                    fflush(file);
                    (*deliveredSkiersCount)++;
                    left = true;
                    sem_post(logSemaphor);
                }
                sem_post(skiersSemaphors[semaphorIndex + 1]);
            }
            fclose(file);
            Free(L);
            return 0;
        }
    }
    fclose(file);
    // Wait for child processes
    while (waitpid(*skibusPId, NULL, 0) > 0)
        ;
    munmap(currentLogNumber, sizeof(int));
    munmap(currentSkibusStopNumber, sizeof(int));
    munmap(finished, sizeof(bool));
    munmap(currentBusPassangersCount, sizeof(int));
    munmap(deliveredSkiersCount, sizeof(int));
    munmap(logSemaphor, sizeof(sem_t));
    munmap(boardSemaphor, sizeof(sem_t));

    for (int i = 0; i < L + 1; i++)
    {
        munmap(skiersSemaphors[i], sizeof(sem_t));
    }

    if (*skibusPId != 0)
    {
        free(skibusPId);
    }

    return 0;
}
