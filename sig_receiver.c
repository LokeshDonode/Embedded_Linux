#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<time.h>
#include<sys/types.h>

#define MYSIGNAL SIGUSR1   // 10
#define MYEXITSIGNAL SIGINT  // 2

void errExit(char *msg){
	perror(msg);
	exit(1);
}

int led_toggle = 1,int_exit = 0,int_sig_received = 0;

static void handler(int sig,siginfo_t *si,void *uc){
	if(sig == MYSIGNAL){
		int_sig_received = 1;
		led_toggle = !led_toggle; // if on turn off and if off turn on
	}
	else if(sig == MYEXITSIGNAL){
		int_sig_received = 1;
		int_exit = 1;
	}
	else{
		printf("Unknown Signal\n");	
	}
} // instead of if-else we can also use switch case for different cases

int main(){
	struct sigaction sa_my,sa_int;
	
	printf("\nSignal Receiver using gpioset\n");
	printf("My Process ID = %d\n",getpid());  // getpid() - returns process id

	// Register SIGUSR1
	sa_my.sa_flags = SA_SIGINFO;
	sa_my.sa_sigaction = handler;
	sigemptyset(&sa_my.sa_mask);
	
	if(sigaction(MYSIGNAL,&sa_my,NULL) == -1){
		errExit("sigaction");
	}
	while(int_exit == 0){
		pause();
	if(int_sig_received == 1){
		if(led_toggle == 0){
			system("gpioset gpiochip1 10=0");
			printf("LED OFF\n");
                }
                else {
                        system("gpioset gpiochip1 10=1");
                        printf("LED ON\n");       
	        }
	        int_sig_received = 0;
           }  //if
        }// while

        printf("\nSignal Receiver : Terminated\n");
	return 0;
}

