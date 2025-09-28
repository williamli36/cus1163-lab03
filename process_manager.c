#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_VALUES 5  // Each producer sends 5 numbers

/*
 * Producer Process - Sends NUM_VALUES sequential numbers starting from start_num
 */
void producer_process(int write_fd, int start_num) {
    setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering

    printf("Producer (PID: %d) starting...\n", getpid());

    for (int i = 0; i < NUM_VALUES; i++) {
        int number = start_num + i;
        if (write(write_fd, &number, sizeof(number)) != sizeof(number)) {
            perror("write");
            exit(1);
        }
        printf("Producer: Sent number %d\n", number);
        usleep(100000); // delay for clarity
    }

    printf("Producer: Finished sending %d numbers\n", NUM_VALUES);
    close(write_fd);
    exit(0);
}

/*
 * Consumer Process - Receives numbers and calculates sum
 */
void consumer_process(int read_fd, int pair_id) {
    setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering

    int number;
    int sum = 0;

    printf("Consumer (PID: %d) starting...\n", getpid());

    while (read(read_fd, &number, sizeof(number)) > 0) {
        sum += number;
        printf("Consumer: Received %d, running sum: %d\n", number, sum);
    }

    printf("Consumer: Final sum: %d\n", sum);
    close(read_fd);
    exit(0);
}

/*
 * Function 1: Basic Producer-Consumer Demo
 */
int run_basic_demo(void) {
    int pipe_fd[2];
    pid_t producer_pid, consumer_pid;
    int status;

    setvbuf(stdout, NULL, _IONBF, 0); // unbuffer parent

    printf("\nStarting basic producer-consumer demonstration...\n");
    printf("Parent process (PID: %d) creating children...\n", getpid());

    // TODO 1: Create pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return -1;
    }

    // TODO 2: Fork producer
    producer_pid = fork();
    if (producer_pid < 0) {
        perror("fork failed");
        return -1;
    }
    if (producer_pid == 0) {
        close(pipe_fd[0]); // close read end
        producer_process(pipe_fd[1], 1);
    } else {
        printf("Created producer child (PID: %d)\n", producer_pid);
    }

    // TODO 3: Fork consumer
    consumer_pid = fork();
    if (consumer_pid < 0) {
        perror("fork failed");
        return -1;
    }
    if (consumer_pid == 0) {
        close(pipe_fd[1]); // close write end
        consumer_process(pipe_fd[0], 0); // pair_id 0 for basic demo
    } else {
        printf("Created consumer child (PID: %d)\n", consumer_pid);
    }

    // TODO 4: Parent cleanup
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    waitpid(producer_pid, &status, 0);
    printf("Producer child (PID: %d) exited with status %d\n", producer_pid, WEXITSTATUS(status));

    waitpid(consumer_pid, &status, 0);
    printf("Consumer child (PID: %d) exited with status %d\n", consumer_pid, WEXITSTATUS(status));

    printf("\nSUCCESS: Basic producer-consumer completed!\n");
    return 0;
}

/*
 * Function 2: Multiple Producer-Consumer Pairs
 */
int run_multiple_pairs(int num_pairs) {
    pid_t pids[40]; // store PIDs
    int pid_count = 0;
    int status;

    setvbuf(stdout, NULL, _IONBF, 0); // unbuffer parent

    printf("\nRunning multiple producer-consumer pairs...\n");
    printf("Parent creating %d producer-consumer pairs...\n", num_pairs);

    for (int i = 0; i < num_pairs; i++) {
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe failed");
            return -1;
        }

        printf("\n=== Pair %d ===\n", i + 1);

        // Fork producer
        pid_t prod_pid = fork();
        if (prod_pid < 0) {
            perror("fork failed");
            return -1;
        }
        if (prod_pid == 0) {
            close(pipe_fd[0]); // close read
            producer_process(pipe_fd[1], i * NUM_VALUES + 1);
        } else {
            pids[pid_count++] = prod_pid;
        }

        // Fork consumer
        pid_t cons_pid = fork();
        if (cons_pid < 0) {
            perror("fork failed");
            return -1;
        }
        if (cons_pid == 0) {
            close(pipe_fd[1]); // close write
            consumer_process(pipe_fd[0], i + 1);
        } else {
            pids[pid_count++] = cons_pid;
        }

        // Parent closes pipe
        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }

    // TODO 6: Wait for all children
    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], &status, 0);
        printf("Child (PID: %d) exited with status %d\n", pids[i], WEXITSTATUS(status));
    }

    printf("\nAll pairs completed successfully!\n");
    printf("\nSUCCESS: Multiple pairs completed!\n");
    return 0;
}
