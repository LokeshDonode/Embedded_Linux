#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct msgbuf{
    long mtype;
    char mtext[100];
}msg_t;

int main(){
    key_t key = ftok("msgfile",65);
    int msgid = msgget(key,0666);

    msg_t message;
    msgrcv(msgid,&message,sizeof(message.mtext),1,0);

    printf("Receiver got: %s\n",message.mtext);

    msgctl(msgid,IPC_RMID,NULL); //delete queue

    return 0;
}