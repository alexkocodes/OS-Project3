/* Group Parners: Alex Ko (fyk211) and Ritin Malhotra (rm5486)*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sem.h>
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
#define MAX_READERS 2 // max number of readers per record
#define BIN_DATA_FILE "Dataset-500.bin"

FILE *fp; // file pointer

long *shared_array = NULL;
sem_t *to_be_destroyed = NULL;
char *name_to_be_destroyed = "/to_be_destroyed";
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
    sem_close(to_be_destroyed);
    sem_unlink(name_to_be_destroyed);
    exit(0);
}

// Function to find a student record in the binary file given the student ID, array index, and output struct
studentRecord *findRecord(long studentID, int index, const long *idArray)
{
    long targetID;
    int targetIndex;
    studentRecord *record = malloc(sizeof(studentRecord));

    targetID = idArray[index];
    targetIndex = index;

    // Move file pointer to the position of the desired record
    fseek(fp, targetIndex * sizeof(studentRecord), SEEK_SET);

    // Read the record at the desired position and check if its ID matches
    if (fread(record, sizeof(studentRecord), 1, fp) == 1 && record->studentID == targetID)
    {
        // Found matching record
        return record;
    }
    else
    {
        // ID not found or read error
        return NULL;
    }
}

// function to check if a write semaphore for this record exists
bool checkWriteSem(char *name)
{
    sem_t *sem_write = sem_open(name, O_CREAT | O_EXCL, 0666, 1);
    if (sem_write == SEM_FAILED)
    {
        if (errno == EEXIST)
        {
            // printf("There is a writer present\n");
            return true;
        }
    }
    else
    {
        sem_close(sem_write);
        sem_unlink(name);
        return false;
    }
    return false;
}

char *getTime()
{
    // get current time in seconds since epoch
    time_t now;
    now = time(NULL);

    char *timestamp = strtok(ctime(&now), "\n");
    return timestamp;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, cleanup_handler); // register the cleanup handler

    char *filename;
    long recid = 0;
    int time_arg;
    char *shmid_input;

    int i = 0;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            filename = argv[i + 1];
            // printf("%s", filename);
        }
        if (strcmp(argv[i], "-l") == 0)
        {
            char *ptr;
            recid = strtol(argv[i + 1], &ptr, 10);
            // printf("%ld", recid);
        }
        if (strcmp(argv[i], "-d") == 0)
        {
            time_arg = atoi(argv[i + 1]);
            // printf("%d", time_arg);
        }
        if (strcmp(argv[i], "-s") == 0)
        {
            shmid_input = argv[i + 1];
            // printf("%s", shmid_input);
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

    // Get the current time (i.e., time at which reader started)
    time_t startTime = time(NULL);

    // open the binary file
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Error opening file\n");
        exit(1);
    }

    // create a text file for logging
    FILE *fptxt;
    fptxt = fopen("log.txt", "a");
    if (fptxt == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    pid_t pid = getpid();
    char *timestamp = getTime();
    fprintf(fptxt, "%s: reader %d enters the session.\n", timestamp, pid);
    fflush(fptxt);
    bool firstPrint = true;
    while (1)
    {

        char *beginning = "/r_";
        char str_recid[20];
        sprintf(str_recid, "%ld", recid);
        char name[strlen(beginning) + strlen(str_recid) + 1];
        strcpy(name, beginning);
        strcat(name, str_recid);

        // check if a write semaphore for this record exists
        char *beginningW = "/w_";
        char str_recidW[20];
        sprintf(str_recidW, "%ld", recid);
        char nameW[strlen(beginningW) + strlen(str_recidW) + 1];
        strcpy(nameW, beginningW);
        strcat(nameW, str_recidW);

        if (checkWriteSem(nameW))
        {
            if (firstPrint)
            {
                printf("There is a writer present\n");
                firstPrint = false;
                timestamp = getTime();
                fprintf(fptxt, "%s: reader %d is waiting because there is a writer present.\n", timestamp, pid);
                fflush(fptxt);
            }
            continue;
        }

        printf("%s status: ", name);
        sem_t *sem_read = sem_open(name, O_CREAT | O_EXCL, 0666, MAX_READERS);

        to_be_destroyed = sem_read;
        name_to_be_destroyed = name;

        if (sem_read == SEM_FAILED)
        {
            if (errno == EEXIST)
            {

                printf("semaphore already exists\n");
                // Semaphore already exists
                sem_read = sem_open(name, 0);
                if (sem_read == SEM_FAILED)
                {
                    perror("sem_open");
                    exit(EXIT_FAILURE);
                }
                // start waiting
                timestamp = getTime();
                fprintf(fptxt, "%s: reader %d is waiting on semaphore %s.\n", timestamp, pid, name);
                fflush(fptxt);
                printf("Waiting on semaphore\n");
                if (sem_wait(sem_read) == -1)
                {
                    perror("Failed to wait on read semaphore");
                    exit(EXIT_FAILURE);
                }
                printf("Semaphore acquired. Now starts reading.\n");
                timestamp = getTime();
                fprintf(fptxt, "%s: reader %d has acquired semaphore %s.\n", timestamp, pid, name);
                fflush(fptxt);

                timestamp = getTime();
                fprintf(fptxt, "%s: reader %d has now begun its work.\n", timestamp, pid);
                fflush(fptxt);

                sleep(time_arg); // sleep for the specified time

                // loop througn the shared memory to find matching student id
                int i = 0;
                bool found = false;
                for (i = 0; i < 500; i++)
                {

                    if (shared_array[i] == recid)
                    {
                        printf("found student record: %ld\n", shared_array[i]);
                        studentRecord *temp_record = findRecord(recid, i, shared_array);
                        if (temp_record != NULL)
                        {
                            printf("%ld %s %s\n", temp_record->studentID, temp_record->lastName, temp_record->firstName);
                        }

                        found = true;
                        // update the records_accessed semaphore by 1
                        sem_t *records_accessed = sem_open("/records_accessed", 0);
                        if (records_accessed == SEM_FAILED)
                        {
                            perror("sem_open");
                            exit(EXIT_FAILURE);
                        }
                        if (sem_post(records_accessed) == -1)
                        {
                            perror("Failed to signal records_accessed semaphore");
                            exit(EXIT_FAILURE);
                        }
                        sem_close(records_accessed);
                    }
                }
                if (found == false)
                {
                    printf("student record not found\n");
                    // printf("recid: %ld\n", recid);
                }

                // done reading
                if (sem_post(sem_read) == -1)
                {
                    perror("Failed to signal read semaphore");
                    exit(EXIT_FAILURE);
                }
                timestamp = getTime();
                fprintf(fptxt, "%s: reader %d has now finished its work.\n", timestamp, pid);
                fflush(fptxt);

                // destroy the semaphores only when the last reader is done
                int val = 0;
                sem_getvalue(sem_read, &val);
                printf("semaphore val: %d\n", val);
                if (val == MAX_READERS) // last reader
                {
                    sem_close(sem_read);
                    sem_unlink(name); // destroy the semaphore
                }
                else
                {
                    sem_close(sem_read); // only close the semaphore
                }
                break;
            }
            else
            {
                perror("sem_open");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // Semaphore did not exist, but we just created it. So we can start reading now.
            printf("semaphore created\n");

            // start waiting
            timestamp = getTime();
            fprintf(fptxt, "%s: reader %d is waiting on semaphore %s.\n", timestamp, pid, name);
            fflush(fptxt);
            printf("Waiting on semaphore\n");
            if (sem_wait(sem_read) == -1)
            {
                perror("Failed to wait on read semaphore");
                exit(EXIT_FAILURE);
            }
            printf("Semaphore acquired. Now starts reading.\n");
            timestamp = getTime();
            fprintf(fptxt, "%s: reader %d has acquired semaphore %s.\n", timestamp, pid, name);
            fflush(fptxt);

            timestamp = getTime();
            fprintf(fptxt, "%s: reader %d has now begun its work.\n", timestamp, pid);
            fflush(fptxt);

            sleep(time_arg);

            // loop througn the shared memory to find matching student id
            int i = 0;
            bool found = false;
            for (i = 0; i < 500; i++)
            {
                if (shared_array[i] == recid)
                {
                    printf("found student record: %ld\n", shared_array[i]);
                    studentRecord *temp_record = findRecord(recid, i, shared_array);
                    if (temp_record != NULL)
                    {
                        printf("%ld %s %s\n", temp_record->studentID, temp_record->lastName, temp_record->firstName);
                    }
                    found = true;
                    // update the records_accessed semaphore by 1
                    sem_t *records_accessed = sem_open("/records_accessed", 0);
                    if (records_accessed == SEM_FAILED)
                    {
                        perror("sem_open");
                        exit(EXIT_FAILURE);
                    }
                    if (sem_post(records_accessed) == -1)
                    {
                        perror("Failed to signal records_accessed semaphore");
                        exit(EXIT_FAILURE);
                    }
                    sem_close(records_accessed);
                }
            }
            if (found == false)
            {
                printf("student record not found\n");
                // printf("recid: %ld\n", recid);
            }

            // done reading
            if (sem_post(sem_read) == -1)
            {
                perror("Failed to signal read semaphore");
                exit(EXIT_FAILURE);
            }
            timestamp = getTime();
            fprintf(fptxt, "%s: reader %d has now finished its work.\n", timestamp, pid);
            fflush(fptxt);

            // destroy the semaphores only when the last reader is done
            int val = 0;
            sem_getvalue(sem_read, &val);
            printf("semaphore val: %d\n", val);
            if (val == MAX_READERS) // last reader
            {
                sem_close(sem_read);
                sem_unlink(name); // destroy the semaphore
            }
            else
            {
                sem_close(sem_read); // only close the semaphore
            }
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

    // Update the readers_encountered semaphore created by the coordinator
    sem_t *sem_readers_encountered = sem_open("/readers_encountered", 0);
    if (sem_readers_encountered == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    if (sem_post(sem_readers_encountered) == -1)
    {
        perror("Failed to signal writers_encountered semaphore");
        exit(EXIT_FAILURE);
    }
    sem_close(sem_readers_encountered);

    // Get the end time
    time_t endTime = time(NULL);
    // Calculate the total time elapsed
    double time_taken = difftime(endTime, startTime);
    printf("time taken: %f\n", time_taken);

    // access the total_reader_time from its shared memory segment and update it (key = 200)
    int shmid_total_reader_time = shmget(200, sizeof(double), 0666);
    if (shmid_total_reader_time == -1)
    {
        printf("accessing total_reader_time shared memory segment failed\n");
        perror("shmget");
        exit(1);
    }
    else
    {
        double *total_reader_time = (double *)shmat(shmid_total_reader_time, NULL, 0);
        if (total_reader_time == (double *)-1)
        {
            printf("attaching to total_reader_time shared memory segment failed\n");
            perror("shmat");
            exit(1);
        }
        else
        {
            *total_reader_time += time_taken;
        }
        // Detach from the shared memory segment
        if (shmdt(total_reader_time) == -1)
        {
            printf("detaching from total_reader_time shared memory segment failed\n");
            perror("shmdt");
            exit(1);
        }
        else
        {
            printf("detaching from total_reader_time shared memory segment successful\n");
        }
    }
    close(fptxt);
    exit(0);
}
