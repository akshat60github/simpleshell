// Features: Piping, Redirection, History, Background Jobs, Built-ins, Shared Memory Logging, etc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_COMMAND_LEN 1024
#define MAX_TOKENS 100
#define MAX_HISTORY 100
#define SHM_NAME "meechan_shared"

struct HistoryInfo {
    char command[MAX_COMMAND_LEN];
    pid_t pid;
    struct timeval start_time;
    struct timeval end_time;
    long duration;
};

struct SharedMem {
    int count;
    struct HistoryInfo history[MAX_HISTORY];
    sem_t mutex;
};

struct SharedMem *shared_mem;
int shm_fd;

void cleanup() {
    sem_destroy(&shared_mem->mutex);
    munmap(shared_mem, sizeof(struct SharedMem));
    shm_unlink(SHM_NAME);
    close(shm_fd);
}

char* format_time(struct timeval tv) {
    time_t raw = tv.tv_sec;
    struct tm *tm_info = localtime(&raw);
    static char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

void write_history(const char *command, pid_t pid, struct timeval start, struct timeval end) {
    sem_wait(&shared_mem->mutex);
    if (shared_mem->count >= MAX_HISTORY) shared_mem->count = 0;
    struct HistoryInfo *entry = &shared_mem->history[shared_mem->count++];
    strncpy(entry->command, command, MAX_COMMAND_LEN);
    entry->pid = pid;
    entry->start_time = start;
    entry->end_time = end;
    entry->duration = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
    sem_post(&shared_mem->mutex);
}

void display_history() {
    sem_wait(&shared_mem->mutex);
    for (int i = 0; i < shared_mem->count; i++) {
        struct HistoryInfo *h = &shared_mem->history[i];
        printf("[%d] %s | PID: %d | Duration: %ldms | Started: %s\n",
               i + 1, h->command, h->pid, h->duration, format_time(h->start_time));
    }
    sem_post(&shared_mem->mutex);
}

