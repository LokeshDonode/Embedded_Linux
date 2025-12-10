#include<stdio.h>
#include<sys/types.h>
#include<sys/msg.h>
#include<string.h>

typedef struct msgbuf{
    long mtype;
    char mtext[100];
}msg_t;

int main(){
    key_t key = ftok("msgfile",65);
    int msgid = msgget(key,0666 | IPC_CREAT);

    msg_t message;
    message.mtype = 1;
    strcpy(message.mtext,"Hello from sender");
    
    msgsnd(msgid,&message,sizeof(message.mtext),0);

    return 0;
}
