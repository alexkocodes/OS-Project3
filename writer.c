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
#define MAX_WRITER 1
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

int read_int_within_timeout(int timeout_seconds, int *result)
{
  char input[256];
  time_t start_time = time(NULL); // Current time
  time_t end_time = start_time + timeout_seconds;
  int fd = fileno(stdin);

  // Set stdin to non-blocking mode
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  printf(">> ");
  fflush(stdout); // Make sure the prompt is actually printed

  // Loop until timeout has elapsed
  while (time(NULL) < end_time)
  {

    // Attempt to read input from the user
    int n = read(fd, input, sizeof(input));

    // Check if input was received
    if (n > 0)
    {
      // Stop reading as soon as the user hits enter
      if (input[n - 1] == '\n')
      {
        input[n - 1] = '\0'; // Replace newline with null terminator
        // Attempt to parse input as an integer
        if (sscanf(input, "%d", result) == 1)
        {
          return 0; // Success
        }
        else
        {
          printf("Invalid input: %s\n", input);
        }
        break; // Stop reading
      }
    }

    // Sleep for 1 second
    sleep(1);
  }

  return -1; // Timeout expired
}

int read_float_within_timeout(int timeout_seconds, float *result)
{
  char input[256];
  time_t start_time = time(NULL); // Current time
  time_t end_time = start_time + timeout_seconds;
  int fd = fileno(stdin);

  // Set stdin to non-blocking mode
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  printf("Please enter the new grade (on a scale of 0.0 to 4.0): ");
  fflush(stdout); // Make sure the prompt is printed immediately

  // Loop until timeout has elapsed
  while (time(NULL) < end_time)
  {

    // Attempt to read input from the user
    int n = read(fd, input, sizeof(input));

    // Check if input was received
    if (n > 0)
    {
      // Check if the user hit enter
      if (input[n - 1] == '\n')
      {
        input[n - 1] = '\0'; // Replace newline with null terminator

        // Attempt to parse input as a float
        if (sscanf(input, "%f", result) == 1)
        {
          return 0; // Success
        }
        else
        {
          printf("Invalid input: %s\n", input);
        }
      }
    }

    // Sleep for 1 second
    sleep(1);
  }

  return -1; // Timeout expired
}

// Function to edit the record in the binary file given the student ID and array index
int editRecord(long studentID, int index, const long *idArray, const double start_time, const int total_process_time)
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
  // Time left to get required input = total_process_time - (time elapsed since start of process)
  int time_left = total_process_time - (time(NULL) - start_time);
  if (read_int_within_timeout(time_left, &courseNum) == -1)
  {
    printf("Timeout expired\n");
    return -2;
  }

  // Get the new grade
  float newGrade;
  time_left = total_process_time - (time(NULL) - start_time);
  if (read_float_within_timeout(time_left, &newGrade) == -1)
  {
    printf("Timeout expired\n");
    return -2;
  }
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

// function to check if a read semaphore for this record exists
bool checkReadSem(char *name)
{
  sem_t *sem_read = sem_open(name, O_CREAT | O_EXCL, 0666, 1);
  if (sem_read == SEM_FAILED)
  {
    if (errno == EEXIST)
    {
      // printf("There is a reader present\n");
      return true;
    }
  }
  else
  {
    sem_close(sem_read);
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
      // printf("%s", recid);
    }
    if (strcmp(argv[i], "-d") == 0)
    {
      time_arg = atoi(argv[i + 1]);
      // printf("%d", time_arg);
    }
    if (strcmp(argv[i], "-s") == 0)
    {
      shmid_input = argv[i + 1];
      // printf("%s", shmid);
    }
  }
  // Cast shmid_input to int
  key_t key = atoi(shmid_input);

  // Locate the shared memory segment
  int shmid = shmget(key, SHM_SIZE, 0666);
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

  // Get starting time of writer process
  time_t startTime = time(NULL);

  // Open binary data file for reading and writing
  fp = fopen(filename, "rb+");

  if (fp == NULL)
  {
    printf("Error opening binary file\n");
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
  fprintf(fptxt, "%s: writer %d enters the session.\n", timestamp, pid);
  fflush(fptxt);

  bool firstPrint = true;
  while (1)
  {

    char *beginning = "/w_";
    char str_recid[20];
    sprintf(str_recid, "%ld", recid);
    char name[strlen(beginning) + strlen(str_recid) + 1];
    strcpy(name, beginning);
    strcat(name, str_recid);

    // check if a read semaphore for this record exists
    char *beginningR = "/r_";
    char str_recidR[20];
    sprintf(str_recidR, "%ld", recid);
    char nameR[strlen(beginningR) + strlen(str_recidR) + 1];
    strcpy(nameR, beginningR);
    strcat(nameR, str_recidR);

    if (checkReadSem(nameR))
    {
      if (firstPrint)
      {
        printf("There is a reader present\n");
        firstPrint = false;
        timestamp = getTime();
        fprintf(fptxt, "%s: writer %d is waiting because there is a reader present.\n", timestamp, pid);
        fflush(fptxt);
      }
      continue;
    }

    printf("%s status: ", name);
    sem_t *sem_writer = sem_open(name, O_CREAT | O_EXCL, 0666, MAX_WRITER);

    to_be_destroyed = sem_writer;
    name_to_be_destroyed = name;
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
        // start writing
        timestamp = getTime();
        fprintf(fptxt, "%s: writer %d is waiting on semaphore %s.\n", timestamp, pid, name);
        fflush(fptxt);
        printf("Waiting on semaphore...\n");
        if (sem_wait(sem_writer) == -1)
        {
          perror("Failed to wait on read semaphore");
          exit(EXIT_FAILURE);
        }
        printf("Semaphore acquired!\n");
        timestamp = getTime();
        fprintf(fptxt, "%s: writer %d has acquired semaphore %s.\n", timestamp, pid, name);
        fflush(fptxt);
        fprintf(fptxt, "%s: writer %d has now begun its work.\n", timestamp, pid);
        fflush(fptxt);
        // loop througn the shared memory to find matching student id
        int i = 0;
        bool found = false;
        for (i = 0; i < 500; i++)
        {

          if (shared_array[i] == recid)
          {
            if (editRecord(recid, i, shared_array, startTime, time_arg) == 0)
            {
              printf("Record updated successfully\n");
              // update the records_modified semaphore by 1
              sem_t *records_modified = sem_open("/records_modified", 0);
              if (records_modified == SEM_FAILED)
              {
                perror("sem_open");
                exit(EXIT_FAILURE);
              }
              if (sem_post(records_modified) == -1)
              {
                perror("Failed to signal records_modified semaphore");
                exit(EXIT_FAILURE);
              }
              sem_close(records_modified);
            }
            else if (editRecord(recid, i, shared_array, startTime, time_arg) == -1)
            {
              printf("Record not found\n");
            }
            else
            {
              // We've run out of time, so we need to exit
              printf("Writer process ran out of time\n");
              goto doneWriting_semExists;
            }
          }
        }
        // Get time left (time_arg - (current time - start time)))
        int timeLeft = time_arg - (time(NULL) - startTime);
        // sleep for the remaining time
        sleep(timeLeft);
      // done writing
      doneWriting_semExists:
        if (sem_post(sem_writer) == -1)
        {
          perror("Failed to signal read semaphore");
          exit(EXIT_FAILURE);
        }
        timestamp = getTime();
        fprintf(fptxt, "%s: writer %d has now finished its work.\n", timestamp, pid);
        fflush(fptxt);
        sem_close(sem_writer);
        sem_unlink(name);
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
      // Semaphore did not exist, but we just created it. So we can start writing now.
      printf("semaphore created\n");

      // start writing
      timestamp = getTime();
      fprintf(fptxt, "%s: writer %d is waiting on semaphore %s.\n", timestamp, pid, name);
      fflush(fptxt);
      printf("Waiting on semaphore...\n");
      if (sem_wait(sem_writer) == -1)
      {
        perror("Failed to wait on read semaphore");
        exit(EXIT_FAILURE);
      }
      printf("Semaphore acquired!\n");
      timestamp = getTime();
      fprintf(fptxt, "%s: writer %d has acquired semaphore %s.\n", timestamp, pid, name);
      fflush(fptxt);
      fprintf(fptxt, "%s: writer %d has now begun its work.\n", timestamp, pid);
      fflush(fptxt);
      // loop througn the shared memory to find matching student id
      int i = 0;
      bool found = false;
      for (i = 0; i < 500; i++)
      {
        if (shared_array[i] == recid)
        {
          if (editRecord(recid, i, shared_array, startTime, time_arg) == 0)
          {
            printf("Record edited successfully\n");
            // update the records_modified semaphore by 1
            sem_t *records_modified = sem_open("/records_modified", 0);
            if (records_modified == SEM_FAILED)
            {
              perror("sem_open");
              exit(EXIT_FAILURE);
            }
            if (sem_post(records_modified) == -1)
            {
              perror("Failed to signal records_modified semaphore");
              exit(EXIT_FAILURE);
            }
            sem_close(records_modified);
          }
          else if (editRecord(recid, i, shared_array, startTime, time_arg) == -1)
          {
            printf("Record not found\n");
          }
          else
          {
            // We've run out of time, so we need to exit
            printf("Writer process ran out of time\n");
            goto doneWriting_semDidNotExist;
          }
        }
      }
      // Get time left (time_arg - (current time - start time)))
      int timeLeft = time_arg - (time(NULL) - startTime);
      // sleep for the remaining time
      sleep(timeLeft);

    // done writing
    doneWriting_semDidNotExist:
      if (sem_post(sem_writer) == -1)
      {
        perror("Failed to signal read semaphore");
        exit(EXIT_FAILURE);
      }
      timestamp = getTime();
      fprintf(fptxt, "%s: writer %d has now finished its work.\n", timestamp, pid);
      fflush(fptxt);
      // destroy the semaphores only when the last reader is done
      sem_close(sem_writer);
      sem_unlink(name);
      break;
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

  // Update the writers_encountered semaphore created by the coordinator
  sem_t *sem_writers_encountered = sem_open("/writers_encountered", 0);
  if (sem_writers_encountered == SEM_FAILED)
  {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  if (sem_post(sem_writers_encountered) == -1)
  {
    perror("Failed to signal writers_encountered semaphore");
    exit(EXIT_FAILURE);
  }
  sem_close(sem_writers_encountered);

  // Get the end time
  time_t endTime = time(NULL);
  // Get time elapsed in seconds
  double time_elapsed = difftime(endTime, startTime);

  // Access the total_writer_time from its shared memory segment (key 300)
  int shmid_total_writer_time = shmget(300, sizeof(double), 0666);
  if (shmid_total_writer_time == -1)
  {
    printf("accessing total_writer_time shared memory segment failed\n");
    perror("shmget");
    exit(1);
  }
  else
  {
    // Update the total_writer_time
    double *total_writer_time = (double *)shmat(shmid_total_writer_time, NULL, 0);
    if (total_writer_time == (double *)-1)
    {
      printf("attaching to total_writer_time shared memory segment failed\n");
      perror("shmat");
      exit(1);
    }
    else
    {
      *total_writer_time += time_elapsed;
      printf("total_writer_time: %f\n", *total_writer_time);
    }

    // Detach from the shared memory segment
    if (shmdt(total_writer_time) == -1)
    {
      printf("detaching from total_writer_time shared memory segment failed\n");
      perror("shmdt");
      exit(1);
    }
    else
    {
      printf("detaching from total_writer_time shared memory segment successful\n");
    }
  }

  exit(0);
}
