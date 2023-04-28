#ifndef COMMON_H
#define COMMON_H

// Global variables for student records
#define ID_LENGTH 9 // Deprecated
#define NAME_LENGTH 20
#define NUM_COURSES 8

// Struct to hold information about a voter to be used as value in hash table
typedef struct
{
  long studentID;
  char lastName[NAME_LENGTH];
  char firstName[NAME_LENGTH];
  float grades[NUM_COURSES];
  float GPA;
} studentRecord;

#endif // COMMON_H