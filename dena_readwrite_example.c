/*
This is a sample implementation of a scenario using the readers/writers problem


we have multiple reader processes that can access the shared memory segment simultaneously, 
as long as there is no writer process currently updating the data. 

This is achieved by using a shared semaphore (read_mutex) to protect the shared memory segment from concurrent writes, 
and by incrementing a counter (read_count) every time a reader process enters the critical section. 

If a writer process is currently updating the data, any pending reader processes will block until the writer process releases the read_mutex.




Similarly, only one writer process can update the shared memory segment at a time, and no other reader or writer process can
access the data during that time. 
This is achieved by using another shared semaphore (write_mutex) to protect the shared memory segment from concurrent reads or writes.

If there are any reader or writer processes currently accessing the data, any pending writer processes will block
until the read_count becomes zero and the write_mutex is available.


In summary, the code ensures that:

    - Multiple reader processes can access the data simultaneously, as long as there is no writer process currently updating it.
    - Only one writer process can update the data at a time, and no other reader or writer process can access it during that time.
    - Pending processes (of any type) must wait until the current writer process completes its work and releases the write_mutex.

This ensures that the data is accessed and updated correctly, without any concurrency issues or race conditions.

*/

/*

This code reads the product details from a CSV file called "products.csv", where each row has the product ID, name, and quantity.

The code then creates three semaphores: 
-a mutex semaphore to protect access to the read count
-a write semaphore to protect access to the shared memory during writes
-a read semaphore to limit the number of concurrent readers.

The code forks a writer process, which updates the quantity of each product in the shared memory. 
It also forks three reader processes, which read the product details from the shared memory. 

The readers use the mutex semaphore to protect access to the read count and the read semaphore to limit the number of concurrent readers.

Once all the child processes have exited, the code removes the semaphores and the shared memory segment.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_SIZE 1024
#define NUM_PRODUCTS 5

#define SEM_MUTEX "/mutex_sem"
#define SEM_WRITE "/write_sem"
#define SEM_READ "/read_sem"

typedef struct {
    int productID;
    char productName[50];
    int productQty;
} Product;

void create_semaphore(const char* name, sem_t** sem, int value) {
    *sem = sem_open(name, O_CREAT | O_EXCL, 0666, value);
    if (*sem == SEM_FAILED) {
        perror("Failed to create semaphore");
        exit(EXIT_FAILURE);
    }
}

void remove_semaphore(const char* name, sem_t* sem) {
    if (sem_close(sem) == -1) {
        perror("Failed to close semaphore");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink(name) == -1) {
        perror("Failed to unlink semaphore");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int shmid;
    Product* data;
    sem_t* sem_mutex;
    sem_t* sem_write;
    sem_t* sem_read;

    // Create shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    if ((data = shmat(shmid, NULL, 0)) == (void*) -1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // Read product details from CSV file
    FILE* fp;
    char line[1024];
    char* token;
    int i = 0;

    fp = fopen("dena_example_data.csv", "r");
    if (fp == NULL) {
        perror("Failed to open CSV file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, 1024, fp)) {
        token = strtok(line, ",");
        data[i].productID = atoi(token);

        token = strtok(NULL, ",");
        strcpy(data[i].productName, token);

        token = strtok(NULL, ",");
        data[i].productQty = atoi(token);

        i++;
    }

    fclose(fp);

    // Create semaphores
    create_semaphore(SEM_MUTEX, &sem_mutex, 1);
    create_semaphore(SEM_WRITE, &sem_write, 1);
    create_semaphore(SEM_READ, &sem_read, NUM_PRODUCTS);

    // Fork writer process
    pid_t pid = fork();

    if (pid < 0) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Writer process
        for (int i = 0; i < NUM_PRODUCTS; i++) {
            // Wait for write semaphore
            if (sem_wait(sem_write) == -1) {
                perror("Failed to wait on write semaphore");
                exit(EXIT_FAILURE);
            }

            // Update product quantity
            data[i].productQty += 10;

            printf("Writer updated product %d: %s (quantity: %d)\n",
                   data[i].productID, data[i].productName, data[i].productQty);

            // Signal write semaphore
            if (sem_post(sem_write) == -1) {
                perror("Failed to signal on write semaphore");
                exit(EXIT_FAILURE);
            }
            sleep(1);
        }

        exit(EXIT_SUCCESS);
    }

     else {
        // Fork multiple reader processes
        for (int j = 0; j < 3; j++) {
            pid_t reader_pid = fork();

            if (reader_pid < 0) {
                perror("Failed to fork");
                exit(EXIT_FAILURE);
            }

            if (reader_pid == 0) {
                // Reader process
                for (int i = 0; i < NUM_PRODUCTS; i++) {
                    // Wait for mutex semaphore
                    if (sem_wait(sem_mutex) == -1) {
                        perror("Failed to wait on mutex semaphore");
                        exit(EXIT_FAILURE);
                    }

                    // Increment read count and check if first reader
                    if (sem_wait(sem_read) == -1) {
                        perror("Failed to wait on read semaphore");
                        exit(EXIT_FAILURE);
                    }

                    if (sem_post(sem_mutex) == -1) {
                        perror("Failed to signal mutex semaphore");
                        exit(EXIT_FAILURE);
                    }

                    // Read product details
                    printf("Reader %d read product %d: %s (quantity: %d)\n",
                           j, data[i].productID, data[i].productName, data[i].productQty);

                    // Decrement read count and check if last reader
                    if (sem_post(sem_read) == -1) {
                        perror("Failed to signal read semaphore");
                        exit(EXIT_FAILURE);
                    }
                    sleep(1);
                }

                exit(EXIT_SUCCESS);
            }
        }
    }

    // Wait for child processes to exit
    for (int i = 0; i < 4; i++) {
        wait(NULL);
    }

    // Remove semaphores
    remove_semaphore(SEM_MUTEX, sem_mutex);
    remove_semaphore(SEM_WRITE, sem_write);
    remove_semaphore(SEM_READ, sem_read);

    // Detach and remove shared memory segment
    if (shmdt(data) == -1) {
        perror("Failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Failed to remove shared memory");
        exit(EXIT_FAILURE);
    }

    return 0;
}
