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
#include "Hashmap.h"

#define SHM_SIZE 1024

// create a hashmap
hash_table *h_table;
void cleanup_handler(int sig)
{

    // Detach from the shared memory segment
    if (shmdt(h_table) == -1)
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
    h_table = init_hash_table();
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
    key = 9999;

    // Locate the shared memory segment
    shmid = shmget(key, SHM_SIZE, 0666);
    // check for faiure (no segment found with that key)
    if (shmid < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach shared memory segment
    if ((h_table = shmat(shmid, NULL, 0)) == (void *)-1)
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // // Search for a student
    // studentRecord *sr = search(h_table, "0#01a7c3");
    // if (sr != NULL)
    // {
    //     printf("Found student: %s, %s\n", sr->lastName, sr->firstName);
    // }
    // else
    // {
    //     printf("Student not found!\n");
    // }

    print_hash_table(h_table);

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
    };
    // Detach from the shared memory segment
    if (shmdt(h_table) == -1)
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
