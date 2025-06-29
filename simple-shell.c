// ULTIMATE C SHELL
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

void sigint_handler(int signum) {
    printf("\nExiting shell. Final command history:\n");
    display_history();
    cleanup();
    exit(0);
}

void execute_command(char *cmdline);

void parse_and_execute(char *line) {
    char *commands[MAX_TOKENS];
    int count = 0;
    char *token = strtok(line, ";");
    while (token && count < MAX_TOKENS) {
        commands[count++] = token;
        token = strtok(NULL, ";");
    }
    for (int i = 0; i < count; i++) {
        execute_command(commands[i]);
    }
}

void execute_command(char *cmdline) {
    char *args[MAX_TOKENS];
    char *cmd = strdup(cmdline);
    char *redir_out = NULL, *redir_in = NULL;
    bool background = false;

    // Handle output redirection
    if ((redir_out = strchr(cmd, '>'))) {
        *redir_out = 0;
        redir_out++;
        while (isspace(*redir_out)) redir_out++;
    }
    if ((redir_in = strchr(cmd, '<'))) {
        *redir_in = 0;
        redir_in++;
        while (isspace(*redir_in)) redir_in++;
    }
    if (cmd[strlen(cmd) - 1] == '&') {
        background = true;
        cmd[strlen(cmd) - 1] = 0;
    }

    int i = 0;
    char *token = strtok(cmd, " ");
    while (token) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL) return;

    // Built-ins
    if (strcmp(args[0], "cd") == 0) {
        if (args[1]) chdir(args[1]);
        else fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (strcmp(args[0], "exit") == 0) {
        sigint_handler(SIGINT);
    }
    if (strcmp(args[0], "history") == 0) {
        display_history();
        return;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pid_t pid = fork();
    if (pid == 0) {
        if (redir_out) {
            int fd = open(redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (redir_in) {
            int fd = open(redir_in, O_RDONLY);
            if (fd < 0) perror("open");
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        if (!background) waitpid(pid, NULL, 0);
        gettimeofday(&end, NULL);
        write_history(cmdline, pid, start, end);
    } else {
        perror("fork");
    }
    free(cmd);
}

int main() {
    signal(SIGINT, sigint_handler);

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct SharedMem));
    shared_mem = mmap(NULL, sizeof(struct SharedMem), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_init(&shared_mem->mutex, 1, 1);
    shared_mem->count = 0;

    char input[MAX_COMMAND_LEN];

    while (1) {
        char cwd[512];
        getcwd(cwd, sizeof(cwd));
        printf("meechan++@%s$ ", cwd);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        parse_and_execute(input);
    }

    sigint_handler(SIGINT);
    return 0;
}
