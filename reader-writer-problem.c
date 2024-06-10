#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>

#define PASSWORD_SIZE 10 // The password table's size
#define MAX_THREADS 10   // Maximum thread count

int BUFFER; // Shared buffer
int passwordTable[PASSWORD_SIZE]; // Array for password storage
sem_t mutex, wrt; // Using semaphores to synchronize
int read_count = 0; // Number of readers using the shared buffer at the moment

typedef struct {
    int id;          // Thread ID
    char role[10];   // Role of threads (reader/writer)
    bool is_real;    // thread is real or dummy
    int password;    // Password to gain entry to the buffer
} thread_info;

void *reader(void *param); // Function of the reader thread
void *writer(void *param); // Function of the writer thread
void initializePasswordTable(); // Initialization function for the password table
bool checkPassword(int password); // feature to verify the validity of the password
void logAccess(int thread_no, bool is_real, char *role, int value, bool has_permission); // The ability to record access attempts

// Reader thread function
void *reader(void *param) {
    thread_info *info = (thread_info *)param;
    for (int i = 0; i < 5; i++) {
        if (checkPassword(info->password)) {
            // Readers' entry section
            sem_wait(&mutex);
            read_count++;
            if (read_count == 1) sem_wait(&wrt);
            sem_post(&mutex);

            // Critical section
            logAccess(info->id, info->is_real, info->role, BUFFER, true);

            // Exit section
            sem_wait(&mutex);
            read_count--;
            if (read_count == 0) sem_post(&wrt);
            sem_post(&mutex);
        } else {
            // If the reader lacks authorization, log
            logAccess(info->id, info->is_real, info->role, -1, false);
        }
        sleep(1); // Sleep
    }
    return NULL;
}

// Writer thread function
void *writer(void *param) {
    thread_info *info = (thread_info *)param;
    for (int i = 0; i < 5; i++) {
        if (checkPassword(info->password)) {
            // Writers' entry section
            sem_wait(&wrt);

            // Critical section
            BUFFER = rand() % 10000; // Write a random value to the buffer
            logAccess(info->id, info->is_real, info->role, BUFFER, true);

            // Exit section
            sem_post(&wrt);
        } else {
            // If the writer lacks authorization, log
            logAccess(info->id, info->is_real, info->role, -1, false);
        }
        sleep(1); // Sleep
    }
    return NULL;
}

// function that generates random passwords to start the password table
void initializePasswordTable() {
    for (int i = 0; i < PASSWORD_SIZE; i++) {
        passwordTable[i] = rand() % 900000 + 100000; // Make random passwords with six digits.
    }
}

// feature to verify the validity of the password entered
bool checkPassword(int password) {
    for (int i = 0; i < PASSWORD_SIZE; i++) {
        if (passwordTable[i] == password) {
            return true; // Password is valid
        }
    }
    return false; // Password is invalid
}

// The ability to record access attempts
void logAccess(int thread_no, bool is_real, char *role, int value, bool has_permission) {
    if (has_permission) {
        printf("%d\t\t%s\t\t\t%s\t\t\t%d\n", thread_no, is_real ? "real" : "dummy", role, value);
    } else {
        printf("%d\t\t%s\t\t\t%s\t\t\tNo permission\n", thread_no, is_real ? "real" : "dummy", role);
    }
}

int main() {
    int num_readers = 5; // Number real reader threads
    int num_writers = 5; // Number real writer threads

    // Initialize semaphores
    sem_init(&mutex, 0, 1);
    sem_init(&wrt, 0, 1);
    initializePasswordTable(); // Initialize password table

    // Print the log's header.
    printf("Number of writers: %d\n", num_writers);
    printf("Number of readers: %d\n", num_readers);
    printf("Thread No\tValidity(real/dummy)\tRole(reader/writer)\tValue read/written\n");

    // Arrays containing thread IDs and thread data
    pthread_t readers[MAX_THREADS], writers[MAX_THREADS], dummy_readers[MAX_THREADS], dummy_writers[MAX_THREADS];
    thread_info reader_info[MAX_THREADS], writer_info[MAX_THREADS], dummy_reader_info[MAX_THREADS], dummy_writer_info[MAX_THREADS];

    // Create real reader threads
    for (int i = 0; i < num_readers; i++) {
        reader_info[i] = (thread_info){.id = i, .role = "Reader", .is_real = true, .password = passwordTable[i]};
        pthread_create(&readers[i], NULL, reader, &reader_info[i]);
    }

    // Create real writer threads
    for (int i = 0; i < num_writers; i++) {
        writer_info[i] = (thread_info){.id = i, .role = "Writer", .is_real = true, .password = passwordTable[i]};
        pthread_create(&writers[i], NULL, writer, &writer_info[i]);
    }

    // Create dummy reader threads
    for (int i = 0; i < num_readers; i++) {
        dummy_reader_info[i] = (thread_info){.id = i, .role = "Reader", .is_real = false, .password = rand() % 900000 + 100000};
        pthread_create(&dummy_readers[i], NULL, reader, &dummy_reader_info[i]);
    }

    // Create dummy writer threads
    for (int i = 0; i < num_writers; i++) {
        dummy_writer_info[i] = (thread_info){.id = i, .role = "Writer", .is_real = false, .password = rand() % 900000 + 100000};
        pthread_create(&dummy_writers[i], NULL, writer, &dummy_writer_info[i]);
    }

    // Wait for all reader threads to finish
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
        pthread_join(dummy_readers[i], NULL);
    }

    // Wait for all writer threads to finish
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
        pthread_join(dummy_writers[i], NULL);
    }

    // Destroy semaphores
    sem_destroy(&mutex);
    sem_destroy(&wrt);

    return 0;
}
