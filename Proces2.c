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
#define SEM2_NAME "sem2"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)


static int koniec = 0;
static int rob = 1;
static int znaki = 0;
sem_t *semafor2;

void stop(){
    printf("P2: STOP\n");
    if(sem_wait(semafor2) < 0){
        perror("sem_wait failed on P2");
    }
    rob=0;
}

void go(){
    printf("P2: START\n");
    if (sem_post(semafor2) < 0) {
        perror("sem_post error on P2");
    }
    rob=1;
}

void end(){
    printf("P2: KONIEC, znaki odebrane: %d\n", znaki);
    koniec=1;
    rob = 0;
    if (sem_post(semafor2) < 0) {
        perror("sem_post error on P2");
    }
    kill(getppid(),SIGQUIT);
}

void zakoncz(){
    int i,j;
    char b[2]="1";
    kill(getpid()-1,SIGINT);
    kill(getpid()+1,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo3",O_WRONLY);
    write(j,b,2);
    close(j);
    end();
}
void zatrzymaj(){
    int i,j;
    char b[2]="2";
    kill(getpid()-1,SIGINT);
    kill(getpid()+1,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo3",O_WRONLY);
    write(j,b,2);
    close(j);
    stop();
}
void wznow(){
    int i,j;
    char b[2]="3";
    kill(getpid()-1,SIGINT);
    kill(getpid()+1,SIGINT);
    i=open("fifo1",O_WRONLY);
    write(i,b,2);
    close(i);
    j=open("fifo3",O_WRONLY);
    write(j,b,2);
    close(j);
    go();
}

void co_robic(){
    int i,j;
    char buf[2];
    i=open("fifo2",O_RDONLY);
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

    if (sem_unlink("semafor2") < 0){
        //perror("sem_unlink(3) failed");
        }

    semafor2 = sem_open("semafor2", O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (semafor2 == SEM_FAILED) {
        perror("sem_open(3) 1 error P2");
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

    sem_t *sem3 = sem_open(SEM3_NAME, O_RDWR);
    if (sem3 == SEM_FAILED) {
        perror("sem_open(3) 3 failed P2");
        exit(EXIT_FAILURE);
    }
    sem_t *sem2 = sem_open(SEM2_NAME, O_RDWR);
    if (sem2 == SEM_FAILED) {
        perror("sem_open(3) 2 failed P2");
        exit(EXIT_FAILURE);
    }

    shmid = shmget(45286, sizeof(char), 0600);  //utworzenie segmentu pamięci współdzielonej z pojemnością na 100 charów
    if(shmid == -1){
        perror("Błąd przyłączenia segmentu pamieci wspoldzielonej P2");
        exit(1);
    }

    buf = shmat(shmid, NULL, 0);
    if(buf == NULL){
        perror("Błąd przyłączania segmentu pamięci współdzielonej P2");
    }

    msgkey = ftok(".", 'm');
    if((qid = msgget(msgkey, 0660 | IPC_CREAT)) == -1) {  //otwarcie kolejki komunikatów
        perror("Blad otwierania kolejki komunikatow P2\n");
        exit(1);
    }

    while(koniec == 0){
        if(rob == 1){
        if (sem_wait(sem2) < 0) {
            perror("sem_wait(3) failed on P2");
            continue;
        }
        if(sem_wait(semafor2) < 0){
            perror("sem_wait failed on P2");
            continue;
        }
        if(koniec == 0){
        printf("P2 - Odebrałem z pamięci: %s\n", buf);
        int len = strlen(buf);
        char hex_str[(len*2)+1];
        int loop = 0, i = 0;
        while(buf[loop] != '\0'){
            sprintf((char*)(hex_str+i), "%02X", buf[loop]);
            loop+=1;
            i+=2;
            znaki++;
        }
        hex_str[i++] = '\0';
        strcpy(buf, hex_str);

        char in[1];

        loop = 0;
        printf("P2 - wysyłam: ");
        while(buf[loop] != '\0'){
            in[0] = buf[loop];
            if(buf[loop] == '\0'){
                queue.mtype = 3;
                queue.i = -1;
                break;
            }
            else{
                queue.mtype = 3;
                queue.i = in[0];
                if((msgsnd(qid, &queue, sizeof(struct mymsgbuf) - sizeof(long), 0)) == -1) {
                    perror("Blad wysylania komunikatu");
                    exit (1);
                }
                printf("%c", *in);
            }
        loop++;
        }
        printf("\n");

        sleep(1);
        if (sem_post(semafor2) < 0) {
            perror("sem_post error on P2");
        }
        if (sem_post(sem3) < 0) {
            perror("sem_post(3) error on P2");
        }
        }

        }

        }

    if (sem_close(semafor2) < 0){
        perror("sem_close(3) sem failed P2");}
    if (sem_close(sem3) < 0){
        perror("sem_close(3) 3 failed P2");}
    if (sem_close(sem2) < 0){
        perror("sem_close(3) 2 failed P2");}
    if (sem_unlink("semafor2") < 0){
        perror("sem_unlink(3) failed");}

    sleep(1);
    shmdt(buf);

return 0;
}
