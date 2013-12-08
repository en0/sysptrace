#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "calls.h"

/** Print the system call and the register state to the **
 ** screen. This is for ARM only. Will not function in  **
 ** any other arch that i know of.                      **
 ** ARGS:                                               **
        r - The state of the registers.                 **
 ** RETURNS:                                            **
 **     the system call integer.                        **/
int arm_printregs(struct user_regs r) {
    /** This function is not portable **/
    int i;
    int c = r.uregs[7];

    if(c == 0) return 0;

    fprintf(stderr, "--------------------------------\n");
    if(c < MAX_CALLS) fprintf(stderr, "SYSCALL: %s (%i)\n", CALL(c), c);
    else fprintf(stderr, "SYSCALL: UKNW (%i)\n", c);
    fprintf(stderr, "--------------------------------\n");
    fprintf(stderr, "Registers:\n");
    for(i = 0; i < 13; i++) {
        fprintf(stderr, "\tr%i%s = 0x%.8x (%i)\n", 
            i, (i < 10 ? "  " : " "),
            r.uregs[i], r.uregs[i]);
    }

    fprintf(stderr, "\tsp   = 0x%.8x (%i)\n", r.uregs[13], r.uregs[13]);
    fprintf(stderr, "\tlr   = 0x%.8x (%i)\n", r.uregs[14], r.uregs[14]);
    fprintf(stderr, "\tpc   = 0x%.8x (%i)\n", r.uregs[15], r.uregs[15]);
    fprintf(stderr, "\tcpsr = 0x%.8x (%i)\n", r.uregs[16], r.uregs[16]);

    return c;
}

/** Print the return code of the syscall. Ignore if         **
 ** privious call was 0. This is an artifact left by        **
 ** when the syscall trace was issued                       **
 ** ARGS:                                                   **
 **     call - the privious system call number              **
 **     r - The register state.                             **/
void arm_printreturn(int call, struct user_regs r) {
    /** This function is not portable **/
    if(call == 0) return;
    fprintf(stderr, "\tRETURN %.8x (%i)\n\n", r.uregs[0], r.uregs[0]);
}

/** Capture all system calls from the child application and **
 ** print the details to stderr.                            **
 ** ARGS:                                                   **
 **     child_pid - The child process id to track.          **/
void trace(pid_t child_pid) {
    struct user_regs uregs;
    int call = 0;
    int status;

    wait(NULL);
    ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
    waitpid(child_pid, &status, 0);

    while (!WIFEXITED(status)) {
        ptrace(PTRACE_GETREGS, child_pid, 0, &uregs);

        // Save state to track call from return
        if(call == 0) {
            call = arm_printregs(uregs);
        } else {
            arm_printreturn(call, uregs);
            call = 0;
        }
            
        ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
        waitpid(child_pid, &status, 0);
    } 
}

/** Attach the ptrace and start the child application   **
 ** ARGS:                                               **
 **     argv - The command line for the child app       **/
void envoke(char** argv) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    kill(getpid(), SIGINT); // Wait for ptrace to attache
    execvp(argv[0], argv);
    fprintf(stderr, "Failed to load %s\n", argv[0]);
    _exit(1);
}

int main(int argc, char** argv) {
    argv++; argc--;
    if(argc == 0) {
        printf("Usage: sysptrace <program> [<args> ...]\n");
        return 1;
    }

    pid_t pid = fork();

    switch(pid) {
        case -1:
            fprintf(stderr, "Failed to fork process!\n");
            return 1;

        case 0:
            envoke(argv);
            break;

        default:
            trace(pid);
            break;
    }

    return 0;
}

