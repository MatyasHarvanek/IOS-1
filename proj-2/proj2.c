
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdbool.h>

int skyierCount = 10;
int targetStopsMaxCount = 10;
int targetStopsCount = 5;

int currentskier;
int capacity = 5;

int skyBusPid;

int *currentLogNumber;
int *currentSkiBusStop;
int *currentNumberOfPassagers;
int *delivered;
bool *busIsInStop;

sem_t *logSemaphor;
sem_t *boardSemaphor;

sem_t **checkSemaphor;

int getRandomNumber(int min, int max, int offset)
{
    srand(time(NULL) + offset);
    int randomNumber = rand() % (max - min + 1) + min;
    return randomNumber;
}

int CreateBus()
{
    skyBusPid = fork();
    if (skyBusPid == 0)
    {
        sem_wait(logSemaphor);
        printf("%d: BUS: started \n", *currentLogNumber);
        sem_post(logSemaphor);
        while (*delivered != skyierCount)
        {
            for (int i = 0; i < targetStopsCount + 1; i++)
            {

                usleep(getRandomNumber(0, 1000, 0));
                if (i != targetStopsCount)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    printf("%d: BUS: arrived to %d\n", *currentLogNumber, i + 1);
                    fflush(stdout);
                    sem_post(logSemaphor);
                }
                else
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    printf("%d: BUS: arrived to final\n", *currentLogNumber);
                    fflush(stdout);
                    sem_post(logSemaphor);
                }

                *currentSkiBusStop = i + 1;
                for (int i = 0; i < skyierCount; i++)
                {
                    sem_post(boardSemaphor);
                }
                sem_post(checkSemaphor[0]);
                sem_wait(checkSemaphor[skyierCount]);

                if (i != targetStopsCount)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    printf("%d: BUS: leaving %d\n", *currentLogNumber, i + 1);
                    fflush(stdout);
                    sem_post(logSemaphor);
                }
            }
        }

        return 1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    currentLogNumber = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    currentSkiBusStop = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    currentNumberOfPassagers = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    delivered = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    busIsInStop = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    logSemaphor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    boardSemaphor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    checkSemaphor = mmap(NULL, sizeof(sem_t *) * (skyierCount + 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    *currentLogNumber = 1;

    sem_init(logSemaphor, 1, 1);
    sem_init(boardSemaphor, 1, 0);
    for (int i = 0; i < skyierCount + 1; i++)
    {
        checkSemaphor[i] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
        sem_init(checkSemaphor[i], 1, 0);
    }

    if (CreateBus())
    {
        return 0;
    }

    for (int i = 0; i < skyierCount; i++)
    {
        int currentSkiPid = fork();
        int currentSkiIndex = i + 1;
        int stop = getRandomNumber(1, targetStopsCount, currentSkiIndex);
        int semaphorIndex = i;
        bool boarded = false;
        bool left = false;
        if (currentSkiPid == 0)
        {

            sem_wait(logSemaphor);
            (*currentLogNumber)++;
            printf("%d: L:%d started \n", *currentLogNumber, currentSkiIndex);
            fflush(stdout);
            sem_post(logSemaphor);

            usleep(getRandomNumber(0, 1000, currentSkiIndex));
            sem_wait(logSemaphor);
            (*currentLogNumber)++;
            printf("%d: L:%d arrived to %d \n", *currentLogNumber, currentSkiIndex, stop);
            fflush(stdout);
            sem_post(logSemaphor);

            while (1)
            {

                sem_wait(checkSemaphor[semaphorIndex]);
                sem_wait(boardSemaphor);

                fflush(stdout);
                // printf("L:%d board Semaphorxd %d \n", currentSkiIndex, stop);
                if (stop == *currentSkiBusStop && !boarded && *currentNumberOfPassagers < capacity)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    printf("%d: L:%d boarding \n", *currentLogNumber, currentSkiIndex);
                    (*currentNumberOfPassagers)++;
                    boarded = true;
                    sem_post(logSemaphor);
                }
                if (boarded && *currentSkiBusStop == targetStopsCount + 1 && !left)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    (*currentNumberOfPassagers)--;
                    printf("%d: L:%d going to ski \n", *currentLogNumber, currentSkiIndex);
                    (*delivered)++;
                    left = true;
                    sem_post(logSemaphor);
                }

                sem_post(checkSemaphor[semaphorIndex + 1]);
            }

            return 0;
        }
    }

    while (wait(NULL) > 0)
        ;
}
