/* Group Parners: Alex Ko (fyk211) and Ritin Malhotra (rm5486)*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include "Hashmap.h"

#define SHM_SIZE 1024

// create a hashmap
studentRecord *shared_array = NULL;
void cleanup_handler(int sig)
{

    // Detach from the shared memory segment
    if (shmdt(shared_array) == -1)
    {
        printf("detaching from shared memory segment failed");
        perror("shmdt");
        exit(1);
    }
    else
    {
        printf("detaching from shared memory segment successful\n");
    }

    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, cleanup_handler); // register the cleanup handler

    char *filename;
    char *recid;
    int time;
    char *shmid_input;

    int i = 0;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            filename = argv[i + 1];
            printf("%s", filename);
        }
        if (strcmp(argv[i], "-l") == 0)
        {
            recid = argv[i + 1];
            printf("%s", recid);
        }
        if (strcmp(argv[i], "-d") == 0)
        {
            time = atoi(argv[i + 1]);
            printf("%d", time);
        }
        if (strcmp(argv[i], "-s") == 0)
        {
            shmid_input = argv[i + 1];
            printf("%s", shmid_input);
        }
    }
    // cast shimid to int
    int shmid = atoi(shmid_input);
    key_t key;

    // The segment with key 9999 that was created by the writer process
    key = 100;

    // Locate the shared memory segment
    shmid = shmget(key, SHM_SIZE, 0666);
    // check for faiure (no segment found with that key)
    if (shmid < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach shared memory segment
    if ((shared_array = shmat(shmid, NULL, 0)) == (void *)-1)
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // read from the shared array
    // printf("%s %s %s\n", shared_array[0].studentID, shared_array[0].firstName, shared_array[0].lastName);

    printf("reader running...\n");
    while (1)
    {

        // take user input and if the user types exit, then we break the while loop
        char input[100];
        printf("Enter a command: ");
        fgets(input, 100, stdin);
        if (strcmp(input, "exit\n") == 0)
        {
            break;
        }
        else if (strcmp(input, "test\n") == 0)
        {
            char *beginning = "/";
            char name[strlen(beginning) + strlen(recid) + 1];
            strcpy(name, beginning);
            strcat(name, recid);
            printf("%s status: ", name);
            sem_t *sem = sem_open(name, O_CREAT | O_EXCL, 0666, 1);
            if (sem == SEM_FAILED)
            {
                if (errno == EEXIST)
                {
                    printf("semaphore already exists\n");
                    // Semaphore already exists
                    sem = sem_open(name, 0);
                    if (sem == SEM_FAILED)
                    {
                        perror("sem_open");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    perror("sem_open");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                // Semaphore did not exist, we created it
                printf("semaphore created\n");
                // destroy the semaphores
                sleep(10);
                sem_close(sem);
                sem_unlink(name);
                printf("semaphore closed\n");
            }
        }
    };
    // Detach from the shared memory segment
    if (shmdt(shared_array) == -1)
    {
        printf("detaching from shared memory segment failed\n");
        perror("shmdt");
        exit(1);
    }
    else
    {
        printf("detaching from shared memory segment successful\n");
    }
    // destroy the shared memory
    shmctl(shmid, IPC_RMID, NULL);

    exit(0);
}
