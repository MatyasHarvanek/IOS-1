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

pid_t *skibusPid;

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
int getRandomNumber(int min, int max, int offset)
{
    srand(time(NULL) + offset);
    return rand() % (max - min + 1) + min;
}

/// @brief Creates process for ski bus with logick
/// @return 0 if it is parent proccess, 1 if it is child proccess
int CreateBus(int L, int Z, int TB, FILE *file)
{
    *skibusPid = fork();
    if (*skibusPid == -1)
    {
        fprintf(stderr, "Error using fork\n");
        return 1;
    }
    if (*skibusPid == 0)
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
                usleep(getRandomNumber(0, TB, i));

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

                // add number of skiers for stop checks
                for (int i = 0; i < L; i++)
                {
                    sem_post(boardSemaphor);
                }

                // start skiers stop check sequencion
                sem_post(skiersSemaphors[0]);

                // wait for finish of skiers stop checks
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

void freeResources(int L)
{
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
    free(skibusPid);
}

int main(int argc, char *argv[])
{
    FILE *file = fopen("proj2.out", "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s L Z K TL TB\n", argv[0]);
        return 1;
    }

    // Převedení vstupních argumentů na čísla
    int L = atoi(argv[1]);  // Počet lyžařů
    int Z = atoi(argv[2]);  // Počet zastávek
    int K = atoi(argv[3]);  // Kapacita skibusu
    int TL = atoi(argv[4]); // Maximální čas čekání lyžařů
    int TB = atoi(argv[5]); // Maximální doba jízdy autobusu

    // Kontrola platnosti vstupních argumentů
    if (L <= 0 || L >= MAX_LYZARU || Z <= 0 || Z > MAX_ZASTAVEK || K < MIN_KAPACITA || K > MAX_KAPACITA || TL < 0 || TL > MAX_TL || TB < 0 || TB > MAX_TB)
    {
        fprintf(stderr, "Chybné vstupní argumenty\n");
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

    if ((currentLogNumber == MAP_FAILED) || (currentSkibusStopNumber == MAP_FAILED) || (finished == MAP_FAILED) || (currentBusPassangersCount == MAP_FAILED) || (deliveredSkiersCount == MAP_FAILED) || (logSemaphor == MAP_FAILED) || (boardSemaphor == MAP_FAILED) || (skiersSemaphors == MAP_FAILED))
    {
        fprintf(stderr, "mmap faild\n");
        freeResources(L);
        return 1;
    }

    skibusPid = malloc(sizeof(pid_t));
    *currentLogNumber = 1;

    sem_init(logSemaphor, 1, 1);
    sem_init(boardSemaphor, 1, 0);

    for (int i = 0; i < L + 1; i++)
    {
        skiersSemaphors[i] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
        sem_init(skiersSemaphors[i], 1, 0);
    }

    if (CreateBus(L, Z, TB, file))
    {
        freeResources(L);
        fclose(file);
        return 0;
    }

    for (int i = 0; i < L; i++)
    {
        int currentSkiPid = fork();
        int currentSkiIndex = i + 1;
        int targetStop = getRandomNumber(1, Z, currentSkiIndex);
        int semaphorIndex = i;
        bool boarded = false;
        bool left = false;

        // check if it is child
        if (currentSkiPid == 0)
        {
            sem_wait(logSemaphor);
            (*currentLogNumber)++;
            fprintf(file, "%d: L %d: started \n", *currentLogNumber, currentSkiIndex);
            fflush(file);
            sem_post(logSemaphor);

            usleep(getRandomNumber(0, TL, currentSkiIndex));
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
            freeResources(L);
            return 0;
        }
    }
    fclose(file);
    while (waitpid(*skibusPid, NULL, 0) > 0)
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
    if (*skibusPid != 0)
    {
        free(skibusPid);
    }
    return 0;
}
