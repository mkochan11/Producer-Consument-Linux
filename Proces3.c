#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ipc.h>

#define SEM3_NAME "sem3"
#define SEM1_NAME "sem1"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

static int koniec = 0;
static int rob = 1;
sem_t *semafor3;
sem_t *sem3;
sem_t *sem1;

void stop(){
    printf("P3: STOP\n");
    if(sem_wait(semafor3) < 0){
        perror("sem_wait failed on P2");
    }
    rob=0;
}

void go(){
    printf("P3: START\n");
    if (sem_post(semafor3) < 0) {
        perror("sem_post error on P2");
    }
    rob=1;
}

void end(){
    printf("P3: KONIEC\n");
    koniec=1;
    rob=0;
    if (sem_post(semafor3) < 0) {
        perror("sem_post error on P3");
    }
    kill(getppid(),SIGQUIT);
}

void zakoncz(){
    int i,j;
    char b[2]="1";
    kill(getpid()-1,SIGINT);
    kill(getpid()-2,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo2",O_WRONLY);
    write(j,b,2);
    close(j);
    end();
}
void zatrzymaj(){
    int i,j;
    char b[2]="2";
    kill(getpid()-1,SIGINT);
    kill(getpid()-2,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo2",O_WRONLY);
    write(j,b,2);
    close(j);
    stop();
}
void wznow(){
    int i,j;
    char b[2]="3";
    kill(getpid()-1,SIGINT);
    kill(getpid()-2,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo2",O_WRONLY);
    write(j,b,2);
    close(j);
    go();
}

void co_robic(){
    int i,j;
    char buf[2];
    i=open("fifo3",O_RDONLY);
    read(i,buf,2);
    close(i);
    j=atoi(buf);
    if(j==1)end();
    if(j==2)stop();
    if(j==3)go();
}


struct mymsgbuf {
    long mtype;
    char i;
}queue ;

struct mymsgbuf readbuffer;

int main(void){

    int semid, shmid;
    char* buf;
    int qid, i;
    key_t msgkey;

    if (sem_unlink("semafor3") < 0){
        //perror("sem_unlink(3) failed");
        }

    semafor3 = sem_open("semafor3", O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (semafor3 == SEM_FAILED) {
        perror("sem_open(3) 1 error P3");
        exit(EXIT_FAILURE);
    }

    sigset_t maskaP;
    sigemptyset(&maskaP);
    sigfillset(&maskaP);
    sigdelset(&maskaP,3);sigdelset(&maskaP,5);sigdelset(&maskaP,14);sigdelset(&maskaP,2);
    sigprocmask(SIG_SETMASK, &maskaP, NULL);

    signal(SIGQUIT,zakoncz);
    signal(SIGTRAP,zatrzymaj);
    signal(SIGALRM,wznow);
    signal(SIGINT,co_robic);

    sem3 = sem_open(SEM3_NAME, O_RDWR);
    if (sem3 == SEM_FAILED) {
        perror("sem_open(3) 3 failed P3");
        exit(EXIT_FAILURE);
    }
    sem1 = sem_open(SEM1_NAME, O_RDWR);
    if (sem1 == SEM_FAILED) {
        perror("sem_open(3) 1 failed P3");
        exit(EXIT_FAILURE);
    }

    msgkey = ftok(".", 'm');
    if((qid = msgget(msgkey, 0660 | IPC_CREAT)) == -1) {  //utworzenie kolejki komunikatów
        perror("Blad otwierania kolejki komunikatow P3\n");
        exit(1);
    }

    while(koniec == 0){
        if(rob == 1){
        if (sem_wait(sem3) < 0) {
            perror("sem_wait(3) failed on P3");
            continue;
        }
        if(sem_wait(semafor3) < 0){
            perror("sem_wait failed on P3");
            continue;
        }
        if(koniec == 0){
        readbuffer.mtype = 3;
        printf("P3 - odczytałem: ");

        while(msgrcv(qid, &readbuffer, sizeof(struct mymsgbuf) - sizeof(long), readbuffer.mtype, IPC_NOWAIT) != -1) {
            printf("%c", readbuffer.i); //odebranie wiadomości i wypisanie jej na ekranie
        }
        printf("\n\n");
        sleep(3);
        if (sem_post(semafor3) < 0) {
            perror("sem_post error on P3");
        }
        if (sem_post(sem1) < 0) {
            perror("sem_post(3) error on P3");
        }
        }
        }
    }

    if (sem_close(semafor3) < 0){
        perror("sem_close(3) sem failed P3");}
    if (sem_close(sem3) < 0){
        perror("sem_close(3) 3 failed P3");}
    if (sem_close(sem1) < 0){
        perror("sem_close(3) 1 failed P3");}
    if (sem_unlink("semafor3") < 0){
        perror("sem_unlink(3) failed");}

return 0;
}
