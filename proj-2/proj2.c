//TODO
//Napsat parsovani argumentu
//napsat free funkci + otestovat s wallgreenem
//Spustit testy pomoci .sh
//prepsat pro sebe 

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdbool.h>

int finalSkyierCount = 10;
int finalStopsCount = 5;
int busCapacity = 5;

pid_t *skyBusPid;

int *currentLogNumber;
int *currentSkiBusStopNumber;
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
int CreateBus()
{
    skyBusPid = fork();
    if (skyBusPid == 0)
    {
        // First log of ski bus
        sem_wait(logSemaphor);
        printf("%d: BUS: started \n", *currentLogNumber);
        sem_post(logSemaphor);

        while (*deliveredSkiersCount != finalSkyierCount)
        {
            // Added one more stop for final stop
            for (int i = 0; i < finalStopsCount + 1; i++)
            {
                usleep(getRandomNumber(0, 1000, i));

                sem_wait(logSemaphor);
                (*currentLogNumber)++;
                if (i != finalStopsCount)
                {
                    printf("%d: BUS: arrived to %d\n", *currentLogNumber, i + 1);
                }
                else
                {
                    printf("%d: BUS: arrived to final\n", *currentLogNumber);
                }
                fflush(stdout);
                sem_post(logSemaphor);

                *currentSkiBusStopNumber = i + 1;

                // add number of skiers for stop checks
                for (int i = 0; i < finalSkyierCount; i++)
                {
                    sem_post(boardSemaphor);
                }

                // start skiers stop check sequencion
                sem_post(skiersSemaphors[0]);

                // wait for finish of skiers stop checks
                sem_wait(skiersSemaphors[finalSkyierCount]);

                if (i != finalStopsCount)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    printf("%d: BUS: leaving %d\n", *currentLogNumber, i + 1);
                    fflush(stdout);
                    sem_post(logSemaphor);
                }
            }
        }
        sem_wait(logSemaphor);
        (*currentLogNumber)++;
        printf("%d: BUS: finished\n", *currentLogNumber);
        sem_post(logSemaphor);
        return 1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    currentLogNumber = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    currentSkiBusStopNumber = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    finished = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    currentBusPassangersCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    deliveredSkiersCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    logSemaphor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    boardSemaphor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    skiersSemaphors = mmap(NULL, sizeof(sem_t *) * (finalSkyierCount + 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    skyBusPid = malloc(sizeof(pid_t));
    *currentLogNumber = 1;

    sem_init(logSemaphor, 1, 1);
    sem_init(boardSemaphor, 1, 0);

    for (int i = 0; i < finalSkyierCount + 1; i++)
    {
        skiersSemaphors[i] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
        sem_init(skiersSemaphors[i], 1, 0);
    }

    if (CreateBus())
    {
        return 0;
    }

    for (int i = 0; i < finalSkyierCount; i++)
    {
        int currentSkiPid = fork();
        int currentSkiIndex = i + 1;
        int targetStop = getRandomNumber(1, finalStopsCount, currentSkiIndex);
        int semaphorIndex = i;
        bool boarded = false;
        bool left = false;

        //check if it is child
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
            printf("%d: L:%d arrived to %d \n", *currentLogNumber, currentSkiIndex, targetStop);
            fflush(stdout);
            sem_post(logSemaphor);

            while (!*finished || !left)
            {
                sem_wait(skiersSemaphors[semaphorIndex]);
                sem_wait(boardSemaphor);

                fflush(stdout);
                if (targetStop == *currentSkiBusStopNumber && !boarded && *currentBusPassangersCount < busCapacity)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    printf("%d: L:%d boarding \n", *currentLogNumber, currentSkiIndex);
                    (*currentBusPassangersCount)++;
                    boarded = true;
                    sem_post(logSemaphor);
                }
                if (boarded && *currentSkiBusStopNumber == finalStopsCount + 1 && !left)
                {
                    sem_wait(logSemaphor);
                    (*currentLogNumber)++;
                    (*currentBusPassangersCount)--;
                    printf("%d: L:%d going to ski \n", *currentLogNumber, currentSkiIndex);
                    (*deliveredSkiersCount)++;
                    left = true;
                    sem_post(logSemaphor);
                }
                sem_post(skiersSemaphors[semaphorIndex + 1]);
            }
            return 0;
        }
    }

    while (wait(skyBusPid) > 0)
        ;
}
