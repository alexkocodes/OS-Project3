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
#define BIN_DATA_FILE "Dataset-copy.bin"
#define MAX_WRITER 1
FILE *fp; // file pointer

// create a hashmap
long *shared_array = NULL;
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

// Function to edit the record in the binary file given the student ID and array index
int editRecord(long studentID, int index, const long *idArray)
{
  printf("----------------------------\n");
  // Call the findRecord function to get the record to edit
  studentRecord *recordToEdit = findRecord(studentID, index, idArray);

  // Check if the record was found
  if (recordToEdit == NULL)
  {
    printf("Record not found\n");
    return -1;
  }
  else
  {
    printf("Record found: ");
    printf("%ld %s %s\n", recordToEdit->studentID, recordToEdit->firstName, recordToEdit->lastName);
  }

  // Place all the GPA values into an array
  float courseGrades[NUM_COURSES];
  printf("Which grade would you like to edit? (enter corresponding number)\n\n");
  for (int i = 0; i < NUM_COURSES; i++)
  {
    courseGrades[i] = recordToEdit->grades[i];
    printf("Course %d: %.2f\n", i, courseGrades[i]);
  }

  // Get the course number to edit
  int courseNum;
  printf(">>");
  scanf("%d", &courseNum);

  // Get the new grade
  float newGrade;
  printf("Please enter the new grade (on a scale of 0.0 to 4.0): ");
  scanf("%f", &newGrade);
  // new grad has to be between 0 to 4
  while (newGrade < 0 || newGrade > 4)
  {
    printf("Please enter a valid grade (on a scale of 0.0 to 4.0): ");
    scanf("%f", &newGrade);
  }

  // Update the record
  recordToEdit->grades[courseNum] = newGrade;

  // Go through the grades array and calculate the new GPA
  float total = 0;
  for (int i = 0; i < NUM_COURSES; i++)
  {
    total += recordToEdit->grades[i];
  }
  recordToEdit->GPA = total / NUM_COURSES;

  // Move file pointer to the position of the desired record
  fseek(fp, index * sizeof(studentRecord), SEEK_SET);

  // Overwrite the record at the desired position
  fwrite(recordToEdit, sizeof(studentRecord), 1, fp);
  return 0;
}

int main(int argc, char *argv[])
{
  signal(SIGINT, cleanup_handler); // register the cleanup handler

  char *filename;
  long recid = 0;
  int time;
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
      // printf("%s", recid);
    }
    if (strcmp(argv[i], "-d") == 0)
    {
      time = atoi(argv[i + 1]);
      // printf("%d", time);
    }
    if (strcmp(argv[i], "-s") == 0)
    {
      shmid_input = argv[i + 1];
      // printf("%s", shmid);
    }
  }
  // Cast shmid_input to int
  int shmid = atoi(shmid_input);
  key_t key;

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

  printf("writer running...\n");

  // Open binary data file for reading and writing
  fp = fopen(filename, "rb+");

  if (fp == NULL)
  {
    printf("Error opening binary file\n");
    exit(1);
  }

  char *beginning = "/";
  char str_recid[20];
  sprintf(str_recid, "%ld", recid);
  char name[strlen(beginning) + strlen(str_recid) + 1];
  strcpy(name, beginning);
  strcat(name, str_recid);
  printf("%s status: ", name);
  sem_t *sem_writer = sem_open(name, O_CREAT | O_EXCL, 0666, MAX_WRITER);
  if (sem_writer == SEM_FAILED)
  {
    if (errno == EEXIST)
    {
      printf("semaphore already exists\n");
      // Semaphore already exists
      sem_writer = sem_open(name, 0);
      if (sem_writer == SEM_FAILED)
      {
        perror("sem_open");
        exit(EXIT_FAILURE);
      }
      // start reading
      printf("Waiting on semaphore...\n");
      if (sem_wait(sem_writer) == -1)
      {
        perror("Failed to wait on read semaphore");
        exit(EXIT_FAILURE);
      }
      printf("Semaphore acquired!\n");

      // loop througn the shared memory to find matching student id
      int i = 0;
      bool found = false;
      for (i = 0; i < 500; i++)
      {

        if (shared_array[i] == recid)
        {
          if (editRecord(recid, i, shared_array) == 0)
          {
            printf("Record updated successfully\n");
          }
          else
          {
            printf("Record not updated\n");
          }
          found = true;
        }
      }
      if (found == false)
      {
        printf("student record not found\n");
      }
      sleep(time);
      // done writing
      if (sem_post(sem_writer) == -1)
      {
        perror("Failed to signal read semaphore");
        exit(EXIT_FAILURE);
      }

      // destroy the semaphores
      sem_close(sem_writer);
      sem_unlink(name);
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

    // start reading
    printf("Waiting on semaphore...\n");
    if (sem_wait(sem_writer) == -1)
    {
      perror("Failed to wait on read semaphore");
      exit(EXIT_FAILURE);
    }
    printf("Semaphore acquired!\n");

    // loop througn the shared memory to find matching student id
    int i = 0;
    bool found = false;
    for (i = 0; i < 500; i++)
    {
      if (shared_array[i] == recid)
      {
        if (editRecord(recid, i, shared_array) == 0)
        {
          printf("Record edited successfully\n");
        }
        else
        {
          printf("Record not edited\n");
        }
        found = true;
      }
    }
    if (found == false)
    {
      printf("student record not found\n");
      // printf("recid: %ld\n", recid);
    }
    sleep(time);

    // done reading
    if (sem_post(sem_writer) == -1)
    {
      perror("Failed to signal read semaphore");
      exit(EXIT_FAILURE);
    }

    // destroy the semaphores
    if (sem_close(sem_writer) == -1)
    {
      perror("Failed to close read semaphore");
      exit(EXIT_FAILURE);
    }
    if (sem_unlink(name) == -1)
    {
      perror("Failed to unlink read semaphore");
      exit(EXIT_FAILURE);
    }
  }

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

  exit(0);
}
