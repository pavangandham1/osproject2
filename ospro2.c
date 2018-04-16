#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#define MAXCP 3
volatile int ccount=0;
void p_action(int sig, siginfo_t *info, void *ptr){
   if (sig == SIGUSR2){
      printf("\ncontroller(%d): Received signal SIGUSR2 from child %d process\n",
            (int)getpid(), (int)info->si_pid);
    }
    --ccount;
 }

 void c_action(int sig){
    if (sig == SIGUSR1){
      printf("\ncompute(%d): Received signal SIGUSR1 from parent process %d\n",
            (int)getpid(), (int)getppid());
    }
 }

int main(){
    int count;
    pid_t pid[MAXCP];
    ccount = MAXCP;

    struct sigaction pact;

    pact.sa_flags = SA_SIGINFO | SA_RESTART; /* if signal comes on top of each other, we need this */
    pact.sa_sigaction = p_action;
    sigaction(SIGUSR2, &pact, NULL);

    /* spawdn children */
    for (count = 0; count < MAXCP; count++){
        switch(pid[count] = fork()){
            case -1:
                perror("Fork error");
                exit(1);
            case 0:
                {
                    struct sigaction cact;
                    cact.sa_flags = 0;
                    cact.sa_handler = c_action;
                    sigaction(SIGUSR1, &cact, NULL);
                    pause();
                    sleep(1);
                    printf("Sending SIGUSR2 to parent (%d)\n", getppid());
                    sleep(count+1);
                    kill(getppid(), SIGUSR2);
                    exit(0);
                    break;
                }
            default:
                break;
        }
    }

    sleep (1); /* let children have time to configure sigaction() */

    /* notify all children */
    for (count = 0; count < MAXCP; count++){
        printf ("Sending SIGUSR1 to child %d\n", (int)pid[count]);
        kill(pid[count], SIGUSR1);
    }

    /* wait for children to notify back */
    while (ccount)
    {
         usleep(10000); /* else CPU throttles */
    }

    for (count = 0; count < MAXCP; count++){
        int status;
        waitpid (pid[count], &status, 0);
        printf ("Child process %d reaped. status=%d\n", pid[count], status);
        kill(pid[count], SIGUSR1);
    }

    return 0;
}

