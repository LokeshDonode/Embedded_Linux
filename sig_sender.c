#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

void errExit(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int opt;
    int signum = -1;
    pid_t pid = -1;

    // Parse options safely
    while ((opt = getopt(argc, argv, "s:p:")) != -1) {
        switch (opt) {
            case 's':
                signum = atoi(optarg);
                break;
            case 'p':
                pid = (pid_t)atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -s <signal_number> -p <process_id>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Check for missing arguments
    if (signum == -1 || pid == -1) {
        fprintf(stderr, "Error: Missing required -s or -p argument.\n");
        fprintf(stderr, "Usage: %s -s <signal_number> -p <process_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Sending signal #%d to process ID %d...\n", signum, pid);

    if (kill(pid, signum) == -1)
        errExit("kill");

    printf("Signal sent successfully.\n");
    return 0;
}

