SimpleShell
Overview
SimpleShell is a lightweight, custom-built Unix shell implemented in C. It supports command execution, piped commands, directory navigation, and a persistent command history stored in shared memory.

Features
Basic Command Execution: Supports execution of standard Linux commands.
Piping: Allows execution of multiple commands connected by pipes (|).
Built-in Commands:
cd (Change directory)
mkdir (Create directories)
history (Display command history)
Command History Tracking:
Stores executed commands along with process ID and execution time.
Uses shared memory (shm_open, mmap) to maintain history across processes.
Signal Handling:
Captures Ctrl+C (SIGINT) to display command history before exit.
Concurrency Management:
Uses semaphores for safe concurrent access to shared memory.
Compilation & Execution
Prerequisites
Ensure you have GCC installed:

sudo apt update && sudo apt install gcc
Compile
gcc simpleshell.c -o simpleshell -lrt -pthread
Run the Shell
./simpleshell
Usage
Upon running, the shell displays a custom prompt:

meechan@UNI:/current/directory$
Running Commands
Execute standard Linux commands, e.g.:

ls -l
echo "Hello, World!"
Using Piped Commands
Commands can be connected via pipes:

ls -l | grep .c | wc -l
Built-in Commands
Change Directory
cd /path/to/directory
Create Directory
mkdir new_folder
View Command History
history
Exiting
Press Ctrl+C to display history and exit safely.

Implementation Details
Shared Memory (shm_open, mmap): Stores history information persistently.
Semaphores (sem_t): Ensures safe concurrent access to shared memory.
Process Management (fork, execvp, wait): Handles command execution and pipelining.
Signal Handling (signal(SIGINT, handler)): Captures Ctrl+C and ensures graceful exit.
Future Enhancements
Add support for background processes (& operator).
Implement output redirection (> and <).
Enhance error handling for unsupported commands.
License
This project is open-source and licensed under the MIT License.

