#include<stdio.h>
#include<unistd.h>
#include<string.h>

int main()
{
    int fd[2];
    pipe(fd);

    if(fork() == 0){
        //Child process --read end
        close(fd[1]);
        char buffer[50];
        read(fd[0],&buffer,sizeof(buffer));
        printf("Child received: %s\n",buffer);
        close(fd[0]);
    }
    else{
        // Parent process --write end
        close(fd[0]);
        char msg[] = "Hello from Parent";
        write(fd[1],msg,strlen(msg)+1);
        close(fd[1]);
    }

    return 0;
}