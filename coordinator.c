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
#define BIN_DATA_FILE "Dataset-500.bin"

long *shared_array = NULL;
double *total_reader_time = NULL;
double *total_writer_time = NULL;
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
    // Detach from the shared memory segment for total_reader_time
    if (shmdt(total_reader_time) == -1)
    {
        printf("detaching from shared memory segment failed");
        perror("shmdt");
        exit(1);
    }
    else
    {
        printf("detaching from shared memory segment successful\n");
    }
    // Detach from the shared memory segment for total_writer_time
    if (shmdt(total_writer_time) == -1)
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

void create_semaphore(const char *name, sem_t **sem, int value)
{
    *sem = sem_open(name, O_CREAT | O_EXCL, 0666, value);
    if (*sem == SEM_FAILED)
    {
        perror("Failed to create semaphore");
        exit(EXIT_FAILURE);
    }
}

void remove_semaphore(const char *name, sem_t *sem)
{
    if (sem_close(sem) == -1)
    {
        perror("Failed to close semaphore");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink(name) == -1)
    {
        perror("Failed to unlink semaphore");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{

    // sem_unlink("/records_accessed");
    // sem_unlink("/records_modified");
    // sem_unlink("/readers_encountered");
    // sem_unlink("/writers_encountered");
    // exit(0);

    // studentRecord *shared_array = NULL;
    signal(SIGINT, cleanup_handler); // register the cleanup handler

    // coordinator creates shared memory segment to store shared data
    key_t key = 100;
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("Failed to create shared memory segment");
        exit(1);
    }
    // Attach shared memory segment
    shared_array = shmat(shmid, NULL, 0);
    if (shared_array == (void *)(-1))
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // Also include two variables in the shared memory segment:
    // 1. readers_encountered: the number of readers that the program has encountered
    // 2. writers_encountered: the number of writers that the program has encountered

    sem_t *readers_encountered;
    sem_t *writers_encountered;
    sem_t *records_accessed;
    sem_t *records_modified;
    create_semaphore("/readers_encountered", &readers_encountered, 0);
    create_semaphore("/writers_encountered", &writers_encountered, 0);
    create_semaphore("/records_accessed", &records_accessed, 0);
    create_semaphore("/records_modified", &records_modified, 0);

    key_t reader_time_key = 200;
    int reader_time_shmid = shmget(reader_time_key, sizeof(double), 0666 | IPC_CREAT);
    if (reader_time_shmid == -1)
    {
        perror("Failed to create shared memory segment");
        exit(1);
    }
    total_reader_time = shmat(reader_time_shmid, NULL, 0);
    // Attach shared memory segment
    if (total_reader_time == (void *)(-1))
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    key_t writer_time_key = 300;
    int writer_time_shmid = shmget(writer_time_key, sizeof(double), 0666 | IPC_CREAT);
    if (writer_time_shmid == -1)
    {
        perror("Failed to create shared memory segment");
        exit(1);
    }
    total_writer_time = shmat(writer_time_shmid, NULL, 0);
    // Attach shared memory segment
    if (total_writer_time == (void *)(-1))
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // open BIN file that contains records
    FILE *fpb;
    long lSize;
    int numOfrecords;
    int i;
    studentRecord rec;
    fpb = fopen(BIN_DATA_FILE, "rb");
    if (fpb == NULL)
    {
        printf("Cannot open binary file\n");
        return (1); // exit if not successful file opening
    }

    // check number of records stats in BIN file
    fseek(fpb, 0, SEEK_END);
    lSize = ftell(fpb);
    rewind(fpb);
    // check abobe lib calls: fseek, ftell, rewind
    numOfrecords = (int)lSize / sizeof(rec);
    // report what is found out of examining the BIN file
    printf("Records found in file %d \n", numOfrecords);
    sleep(1);

    // Read the records from the BIN file and store them in the shared memory
    for (i = 0; i < numOfrecords; i++)
    {
        fread(&rec, sizeof(rec), 1, fpb);
        shared_array[i] = rec.studentID; // now just storing the studentID, but we probably don't need this
        // memcpy(&shared_array[i], &rec, sizeof(rec));
        // printf("%ld %-20s %-20s ",
        //        rec.studentID, rec.lastName, rec.firstName);
        // for (j = 0; j < NUM_COURSES; j++)
        //     printf("%4.2f ", rec.grades[j]);
        // printf("%4.2f\n", rec.GPA);
        // printf("studentID: %ld\n", shared_array[i]);
    }

    fclose(fpb);

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
    // Print out the number of readers and writers encountered
    int readers, writers;

    sem_getvalue(readers_encountered, &readers);
    sem_getvalue(writers_encountered, &writers);
    printf("Number of readers encountered: %d\n", readers);
    printf("Number of writers encountered: %d\n", writers);
    // remove the semaphores
    sem_close(readers_encountered);
    sem_close(writers_encountered);
    sem_unlink("/readers_encountered");
    sem_unlink("/writers_encountered");

    // Print the average time taken by readers and writers
    if (readers > 0) {
    printf("Average time taken by readers: %.2f\n", *total_reader_time / readers);
    } else {
        printf("No readers encountered. Can't compute average duration\n");
    }
    if (writers > 0) {
    printf("Average time taken by writers: %.2f\n", *total_writer_time / writers);
    } else {
        printf("No writers encountered. Can't compute average duration\n");
    }

    // Print the number of records accessed and modified
    int accessed, modified;
    sem_getvalue(records_accessed, &accessed);
    sem_getvalue(records_modified, &modified);
    printf("Number of records accessed: %d\n", accessed);
    printf("Number of records modified: %d\n", modified);

    // destory the semaphores for records
    sem_close(records_accessed);
    sem_close(records_modified);
    sem_unlink("/records_accessed");
    sem_unlink("/records_modified");

    // Detach from the shared memory segment
    if (shmdt(total_reader_time) == -1)
    {
        printf("detaching from shared memory segment failed\n");
        perror("shmdt");
        exit(1);
    }
    else
    {
        printf("detaching from shared memory segment for reader time successful\n");
    }
    // Detach from the shared memory segment
    if (shmdt(total_writer_time) == -1)
    {
        printf("detaching from shared memory segment failed\n");
        perror("shmdt");
        exit(1);
    }
    else
    {
        printf("detaching from shared memory segment for writer time successful\n");
    }

    // destroy the shared memory
    shmctl(reader_time_shmid, IPC_RMID, NULL);
    shmctl(writer_time_shmid, IPC_RMID, NULL);
    exit(0);
}
