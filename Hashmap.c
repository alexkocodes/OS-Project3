#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// ------------------ Global Variables ------------------
// Default values initialized for 50 entries
// Main function will change these values based on command line arguments
int NUM_BUCKETS = 25;
int BUCKET_SIZE = 5;
int H0_CONST = 25;
int H1_CONST = 50;

// Global variables for student records
#define ID_LENGTH 9
#define LAST_NAME_LENGTH 21
#define FIRST_NAME_LENGTH 21
#define NUM_COURSES 8

// Struct to hold information about a voter to be used as value in hash table
typedef struct {
    char studentID[ID_LENGTH];
    char lastName[LAST_NAME_LENGTH];
    char firstName[FIRST_NAME_LENGTH];
    float grades[NUM_COURSES];
    float GPA;
    // int isBeingModified;
} studentRecord;

// Function to encode a string (in our case, the student ID)
// into a long integer (for the hash table containing student records)
// Courtesy of chatGPT and some rough mathematics
long encodeString(char* str) {
    long hash = 5381;
    int c;

    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % 10000;
}


// ------------------ Global Variables ------------------

// ------------------ Linked List Functions ------------------
// Struct to hold information about a node in a linked list
// Key: int (PIN) for hash table
// Value: Voter informaton
typedef struct node {
  char key[ID_LENGTH];
  studentRecord value;
  struct node *next;
} node;

// Function to initialize a new linked list
node *init_linked_list() {
  node *head = NULL;
  head = (node *)malloc(sizeof(node));
  if (head == NULL) {
    return NULL;
  }
  head->next = NULL;
  return head;
}

// Function to get the length of a linked list
int get_length(node *head) {
  int length = 0;
  node *current = head;
  while (current->next != NULL) {
    length++;
    current = current->next;
  }
  return length;
}

// Function to insert a new node at the end of a linked list
// Parameters:
// head: pointer to the head of the linked list
// value: voter_info struct to be inserted
// Key will be the PIN of the voter (value.pin)
node *insert_at_end(node *head, studentRecord value) {
  node *new_node = (node *)malloc(sizeof(node));
  strncpy(new_node->key, value.studentID, ID_LENGTH);
  new_node->value = value;
  new_node->next = NULL;
  node *current = head;
  while (current->next != NULL) {
    current = current->next;
  }
  current->next = new_node;
  return head;
}

// Function to delete a node from the end of a linked list
node *delete_at_end(node *head) {
  node *current = head;
  node *prev = NULL;
  while (current->next != NULL) {
    prev = current;
    current = current->next;
  }
  free(current);
  prev->next = NULL;
  return head;
}

// Function to delete n nodes from the end of a linked list
void delete_nodes_from_end(node *head, int n) {
  node *prev = head;
  node *curr = head->next;
  if (curr == NULL) {
    return;
  }
  int i;
  for (i = 0; i < n; i++) {
    if (curr->next == NULL) {
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  prev->next = NULL;
  while (curr != NULL) {
    node *temp = curr;
    curr = curr->next;
    free(temp);
  }
}

// Function to print the contents of a linked list
void print_linked_list(node *head) {
  node *current = head->next;
  while (current != NULL) {
    printf("Key: %s\n", current->key);
    printf("Value: Student ID: %s, Last Name: %s, First Name: %s, GPA: %f ",
           current->value.studentID, current->value.lastName,
           current->value.firstName, current->value.GPA);
    for (int i = 0; i < NUM_COURSES; i++) {
      printf("%f ", current->value.grades[i]);
    }
    printf("\n");
    current = current->next;
}
}
// ------------------ Linked List Functions ------------------

// ------------------ Hash Table Functions ------------------
// Struct to hold information about a hash table
typedef struct hash_table {
  int size;
  int cutoff;
  node **table;
} hash_table;

// Function to initialize a new hash table
hash_table *init_hash_table() {
  hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
  ht->size = NUM_BUCKETS;
  ht->cutoff = 0;
  ht->table = (node **)malloc(ht->size * sizeof(node *));
  int i;
  for (i = 0; i < ht->size; i++) {
    ht->table[i] = init_linked_list();
  }
  return ht;
}

// Functions to hash a key using the hash function
int hash_0(int key) { return key % H0_CONST; }
int hash_1(int key) { return key % H1_CONST; }

// Function to insert a new node into a hash table
// Parameters:
// ht: pointer to the hash table
// value: voter_info struct to be inserted
// Key will be the PIN of the voter (value.pin)
void insert(hash_table *ht, studentRecord value) {
  // Get the key
  long key = encodeString(value.studentID);
  // Get the index (i.e. bucket) to insert into
  // 1. Use hash_0(key) if it is >= cutoff
  // 2. Use hash_1(key) if it is < cutoff
  int index;
  if (hash_0(key) >= ht->cutoff) {
    index = hash_0(key);
  } else {
    index = hash_1(key);
  }

  // Get bucket at index
  node *bucket = ht->table[index];

  // If bucket is not full (i.e. length < BUCKET_SIZE) insert at end
  if (get_length(bucket) < BUCKET_SIZE) {
    bucket = insert_at_end(bucket, value);
  } else {
    // Add element to the end of the bucket
    bucket = insert_at_end(bucket, value);
    // Increment cutoff
    ht->cutoff++;
    // Split bucket (cutoff - 1) into two:
    // 1. Insert a new bucket at the end of the table
    // 2. Rehash all elements in the bucket (cutoff - 1) using hash_1
    // 3. Insert all elements into new bucket(s) according to hash_1

    // Create new bucket at the end of the table
    node *new_bucket = init_linked_list();
    ht->table[ht->size] = new_bucket;
    ht->size++;

    // Get all elements from bucket (cutoff - 1) and temporarily store in an
    // array
    node *current = ht->table[ht->cutoff - 1]->next;
    int length = get_length(ht->table[ht->cutoff - 1]);
    studentRecord *temp = (studentRecord *)malloc(length * sizeof(studentRecord));
    int i = 0;
    while (current != NULL) {
      temp[i] = current->value;
      current = current->next;
      i++;
    }

    // Delete all elements from bucket (cutoff - 1)
    delete_nodes_from_end(ht->table[ht->cutoff - 1], length);

    // Rehash all elements in the array and insert into new bucket(s)
    for (i = 0; i < length; i++) {
      int index = hash_1(encodeString(temp[i].studentID));
      if (index == ht->cutoff - 1) {
        ht->table[ht->cutoff - 1] =
            insert_at_end(ht->table[ht->cutoff - 1], temp[i]);
      } else {
        ht->table[ht->size - 1] =
            insert_at_end(ht->table[ht->size - 1], temp[i]);
      }
    }
    free(temp);
  }

  // If cutoff >= original size of table:
  // 1. Double the size of the table
  // 2. Modify hash_0 and hash_1 by multiplying constants by 2
  // 3. Reset cutoff to 0
  if (ht->cutoff >= NUM_BUCKETS) {
    ht->size *= 2;
    ht->table = (node **)realloc(ht->table, ht->size * sizeof(node *));
    int i;
    for (i = NUM_BUCKETS; i < ht->size; i++) {
      ht->table[i] = init_linked_list();
    }
    ht->cutoff = 0;
    H0_CONST *= 2;
    H1_CONST *= 2;
  }
}

// Function to search for a voter in the hash table
// Parameters:
// ht: pointer to the hash table
// key: key to search for
// Returns a pointer to a voter_info struct if found, NULL otherwise
studentRecord *search(hash_table *ht, char argKey[9]) {
  // Get the index (i.e. bucket) to search in
  // 1. Use hash_0(key) if it is >= cutoff
  // 2. Use hash_1(key) if it is < cutoff
  long key = encodeString(argKey);
  int index;
  if (hash_0(key) >= ht->cutoff) {
    index = hash_0(key);
  } else {
    index = hash_1(key);
  }
  // Get bucket at index
  node *bucket = ht->table[index];
  // Search for key in bucket
  node *current = bucket->next;
  while (current != NULL) {
    if ((encodeString(current->value.studentID)) == key) {
      return &(current->value);
    }
    current = current->next;
  }
  return NULL;
}

// Function to print the hash table
void print_hash_table(hash_table *ht) {
  int i;
  for (i = 0; i < ht->size; i++) {
    printf("------------------\n");
    printf("Bucket %d:\n", i);
    print_linked_list(ht->table[i]);
  }
}
// ------------------ Hash Table Functions ------------------

void testHashTable() {
    // Create a new hash table
    hash_table *ht = init_hash_table();
    
    // Insert some sample entries
    studentRecord sr1 = {"00000001", "Smith", "John", {4.0, 3.5, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0}, 3.50};
    studentRecord sr2 = {"a1c00002", "Johnson", "Mary", {3.5, 3.5, 3.0, 3.0, 0.0, 0.0, 0.0, 0.0}, 3.25};
    studentRecord sr3 = {"0#01a7c3", "Garcia", "Carlos", {4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0}, 4.00};
    studentRecord sr4 = {"00000004", "Lee", "Jae", {2.0, 1.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0}, 1.25};
    insert(ht, sr1);
    insert(ht, sr2);
    insert(ht, sr3);
    insert(ht, sr4);
    
    // Search for a student
    studentRecord *sr = search(ht, "0#01a7c3");
    if (sr != NULL) {
        printf("Found student: %s, %s\n", sr->lastName, sr->firstName);
    } else {
        printf("Student not found!\n");
    }

    // Print the hash table
    print_hash_table(ht);
}

int main() {
    testHashTable();
    return 0;
}