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

    // Insert some sample entries
    studentRecord sr1 = {"00000001", "Smith", "John", {4.0, 3.5, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0}, 3.50};
    studentRecord sr2 = {"a1c00002", "Johnson", "Mary", {3.5, 3.5, 3.0, 3.0, 0.0, 0.0, 0.0, 0.0}, 3.25};
    studentRecord sr3 = {"0#01a7c3", "Garcia", "Carlos", {4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0}, 4.00};
    studentRecord sr4 = {"00000004", "Lee", "Jae", {2.0, 1.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0}, 1.25};

    // coordinator creates shared memory segment to store the hashmap
    key_t key = 9999;
    int shmid = shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("Failed to create shared memory segment");
        exit(1);
    }
    // Attach shared memory segment
    if ((h_table = shmat(shmid, NULL, 0)) == (hash_table *)-1)
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // Insert the records into the hashmap
    // insert(h_table, sr1);
    // insert(h_table, sr2);
    // insert(h_table, sr3);
    // insert(h_table, sr4);

    print_hash_table(h_table);
    printf("coordinator running...\n");
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
