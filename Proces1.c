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

#define SEM1_NAME "sem1"
#define SEM2_NAME "sem2"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

static int koniec = 0;
static int rob = 1;
sem_t *semafor1;

void stop(){
    printf("P1: STOP\n");
    if(sem_wait(semafor1) < 0){
        perror("sem_wait failed on P1");
    }
    rob=0;
}

void go(){
    printf("P1: START\n");
    if (sem_post(semafor1) < 0) {
        perror("sem_post error on P1");
    }
    rob=1;
}

void end(){
    printf("P1: KONIEC\n");
    koniec=1;
    rob = 0;
    if (sem_post(semafor1) < 0) {
        perror("sem_post error on P1");
    }
    kill(getppid(),SIGQUIT);
}

void zakoncz(){
    int i,j;
    char b[2]="1";
    kill(getpid()+1,SIGINT);
    kill(getpid()+2,SIGINT);
    i=open("fifo2",O_WRONLY);
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
    kill(getpid()+1,SIGINT);
    kill(getpid()+2,SIGINT);
    i=open("fifo2",O_WRONLY);
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
    kill(getpid()+1,SIGINT);
    kill(getpid()+2,SIGINT);
    i=open("fifo2",O_WRONLY);
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
    i=open("fifo1",O_RDONLY);
    read(i,buf,2);
    close(i);
    j=atoi(buf);
    if(j==1)end();
    if(j==2)stop();
    if(j==3)go();
}

int main(void){

    int semid, shmid;
    char* buf;

    if (sem_unlink("semafor1") < 0){
        //perror("sem_unlink(3) failed");
        }

    semafor1 = sem_open("semafor1", O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (semafor1 == SEM_FAILED) {
        perror("sem_open(3) 1 error P1");
        exit(EXIT_FAILURE);
    }

    sigset_t maskaP;
    sigemptyset(&maskaP);
    sigfillset(&maskaP);
    sigdelset(&maskaP,5);sigdelset(&maskaP,14);sigdelset(&maskaP,3);sigdelset(&maskaP,2);
    sigprocmask(SIG_SETMASK, &maskaP, NULL);

    signal(SIGQUIT,zakoncz);
    signal(SIGTRAP,zatrzymaj);
    signal(SIGALRM,wznow);
    signal(SIGINT,co_robic);

    sem_t *sem1 = sem_open(SEM1_NAME, O_RDWR);
    if (sem1 == SEM_FAILED) {
        perror("sem_open(3) 1 failed P1");
        exit(EXIT_FAILURE);
    }
    sem_t *sem2 = sem_open(SEM2_NAME, O_RDWR);
    if (sem2 == SEM_FAILED) {
        perror("sem_open(3) 2 failed P1");
        exit(EXIT_FAILURE);
    }

    shmid = shmget(45286, sizeof(char), 0600);  //utworzenie segmentu pamięci współdzielonej z pojemnością na 100 charów
    if(shmid == -1){
        perror("Błąd przyłączenia segmentu pamieci wspoldzielonej P1");
        exit(1);
    }

    buf = shmat(shmid, NULL, 0);
    if(buf == NULL){
        perror("Błąd przyłączania segmentu pamięci współdzielonej P1");
        exit(1);
    }

    while(koniec == 0){
        if(rob == 1){
        sleep(1);
        if (sem_wait(sem1) < 0) {
            perror("sem_wait(3) failed on P1");
            continue;
        }
        if(sem_wait(semafor1) < 0){
            perror("sem_wait failed on P1");
            continue;
        }
        sem_post(semafor1);
        if(koniec == 0){
        sleep(1);

        int wybor; char dane_k[100]; char * dane_p = 0; char sciezka[100]; int c; FILE *fp; int len = 0;
        printf("Jak chcesz wprowadzić dane?\n   1. Z klawiatury - wpisz 1\n   2. Z pliku - wpisz 2\nWybór:\n ");
        scanf("%d", &wybor);
        getchar();
            if(wybor == 1){
                if(sem_wait(semafor1) < 0){
                    perror("sem_wait failed on P1");
                    continue;
                }
                sem_post(semafor1);
                if(koniec == 0){
                printf("Wpisz dane:\n");
                scanf("%[^\n]s",dane_k); getchar();
                strcpy(buf, dane_k);
                if(sem_wait(semafor1) < 0){
                    perror("sem_wait failed on P1");
                    continue;
                }
                sem_post(semafor1);
                if(koniec == 0){
                printf("P1 - Zapisuje do pamieci: %s\n", buf);
                buf == NULL;
                sleep(2);
                sem_post(semafor1);
                if (sem_post(sem2) < 0) {
                    perror("sem_post(3) error on P1");
                        }
                        }
                    }
                }
            else if(wybor == 2){
                if(sem_wait(semafor1) < 0){
                    perror("sem_wait failed on P1");
                    continue;
                }
                sem_post(semafor1);
                if(koniec == 0){
                printf("Podaj ścieżkę do pliku:\n");
                scanf("%[^\n]s",sciezka); getchar();
                while((fp = fopen(sciezka, "r")) == NULL){
                    printf("Nie mozna otworzyc podanego pliku, spróbuj ponownie:\n");
                    scanf("%[^\n]s",sciezka); getchar();
                }
                fseek(fp, 0, SEEK_END);
                len = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                dane_p = malloc(len);
                if(dane_p){
                    fread(dane_p, 1, len, fp);
                }
                fclose(fp);
                strcpy(buf, dane_p);
                printf("P1 - Zapisuje do pamieci: %s\n", buf);
                    }
                sleep(2);
                sem_post(semafor1);
                if (sem_post(sem2) < 0) {
                    perror("sem_post(3) error on P1");
                    }
                }
            else{
                if(sem_wait(semafor1) < 0){
                    perror("sem_wait failed on P1");
                    continue;
                }
                sem_post(semafor1);
                printf("Nie podano wyboru / podano zły wybór, sprobuj ponownie...\n");
                }
                //sem_post(sem1);
                }

            }
        }
    if (sem_close(semafor1) < 0){
        perror("sem_close(3) 1 failed P1");}
    if (sem_close(sem1) < 0){
        perror("sem_close(3) 1 failed P1");}
    if (sem_close(sem2) < 0){
        perror("sem_close(3) 2 failed P1");}
    if (sem_unlink("semafor1") < 0){
        perror("sem_unlink(3) failed");}
    shmdt(buf);
    sleep(1);

return 0;

}

