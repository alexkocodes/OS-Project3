#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// Default values initialized for 50 entries
// Main function will change these values based on command line arguments
extern int NUM_BUCKETS;
extern int BUCKET_SIZE;
extern int H0_CONST;
extern int H1_CONST;

// Global variables for student records
#define ID_LENGTH 9
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

// Function to encode a string (in our case, the student ID)
// into a long integer (for the hash table containing student records)
// Courtesy of chatGPT and some rough mathematics
long encodeString(char *str);

// Struct to hold information about a node in a linked list
// Key: int (PIN) for hash table
// Value: Voter informaton
typedef struct node
{
  char key[ID_LENGTH];
  studentRecord value;
  struct node *next;
} node;

// Function to initialize a new linked list
node *init_linked_list();

// Function to get the length of a linked list
int get_length(node *head);

// Function to insert a new node at the end of a linked list
// Parameters:
// head: pointer to the head of the linked list
// value: voter_info struct to be inserted
// Key will be the PIN of the voter (value.pin)
node *insert_at_end(node *head, studentRecord value);

// Function to delete a node from the end of a linked list
node *delete_at_end(node *head);

// Function to delete n nodes from the end of a linked list
void delete_nodes_from_end(node *head, int n);

// Function to print the contents of a linked list
void print_linked_list(node *head);

// Struct to hold information about a hash table
typedef struct hash_table
{
  int size;
  int cutoff;
  node **table;
} hash_table;

// Function to initialize a new hash table
hash_table *init_hash_table();

// Functions to hash a key using the hash function
int hash_0(int k);
int hash_1(int k);

// Function to insert a key-value pair into a hash table
void insert(hash_table *ht, studentRecord value);

// Function to search for a value in a hash table given its key
studentRecord *search(hash_table *ht, char argKey[9]);

// Function to print the contents of a hash table
void print_hash_table(hash_table *ht);

#endif /* HASHMAP_H */
