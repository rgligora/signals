#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>


#define STACK_SIZE 10

#define SIGINT_LVL 3
#define SIGUSR1_LVL 2
#define SIGTERM_LVL 1

int T_P = 0;
int K_Z[3] = {0};
int stack[STACK_SIZE];
int top = -1;
bool obradaK_Z = false;

struct timespec t0; 

void postavi_pocetno_vrijeme(){
	clock_gettime(CLOCK_REALTIME, &t0);
}

void vrijeme(void){
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	t.tv_sec -= t0.tv_sec;
	t.tv_nsec -= t0.tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_nsec += 1000000000;
		t.tv_sec--;
	}
	printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

/* ispis kao i printf uz dodatak trenutnog vremena na pocetku */
#define PRINTF(format, ...)       \
do {                              \
  vrijeme();                      \
  printf(format, ##__VA_ARGS__);  \
}                                 \
while(0)

/*
 * spava zadani broj sekundi
 * ako se prekine signalom, kasnije nastavlja spavati neprospavano
 */
void spavaj(time_t sekundi)
{
	struct timespec koliko;
	koliko.tv_sec = sekundi;
	koliko.tv_nsec = 0;

	while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR){
		//PRINTF("");
	}
}

void stanje(){
	PRINTF("K_Z= ");
	for(int i =0;i<3;i++)
		printf("%d",K_Z[i]);
	printf(", T_P= %d, stog: ",T_P);
	if(top == -1){
		printf("-");
	}
	for(int i = top; i >= 0;i--){
		if(i != 0){
			if(i % 2 != 0){
				printf("%d, ",stack[i]);
			}else {
				printf("reg[%d], ",stack[i]);
			}
		} else {
			printf("reg[%d]",stack[i]);
		}
	}
	printf("\n\n");
}

void push_state(int value){
	if(top == STACK_SIZE-1){
		printf("Stackoverflow\n");
		exit(1);
	}
	top++;
	stack[top] = value;
}
void pop_state(){
	if(top == -1){
		printf("Stack underflow!\n");
		exit(1);
	}
	T_P = stack[top];
	top= top - 2;
}

void obradi_sigint(int sig){
	T_P = sig;
	K_Z[sig-1] = 0;
	PRINTF("SIGINT: zapocinjem obradu razine %d\n",sig);
	stanje();
	spavaj(7);
	PRINTF("SIGINT: Zavrsila obrada prekida razine %d\n",sig);
	pop_state();
	if(T_P != 0){
		PRINTF("Nastavlja se obrada prekida razine %d\n", T_P);
	} else {
		PRINTF("Nastavlja se izvodenje glavnog programa\n");
	}
	stanje();
}
void obradi_sigterm(int sig){
	T_P = sig;
	K_Z[sig-1] = 0;
	PRINTF("SIGTERM: zapocinjem obradu razine %d\n",sig);
	stanje();
	spavaj(7);
	PRINTF("SIGTERM: Zavrsila obrada prekida razine %d\n",sig);
	pop_state();
	if(T_P != 0){
		PRINTF("Nastavlja se obrada prekida razine %d\n", T_P);
	} else {
		PRINTF("Nastavlja se izvodenje glavnog programa\n");
	}
	stanje();
}
void obradi_sigusr1(int sig){
	T_P = sig;
	K_Z[sig-1] = 0;
	PRINTF("SIGUSR1: zapocinjem obradu razine %d\n", sig);
	stanje();
	spavaj(7);
	PRINTF("SIGUSR1: Zavrsila obrada prekida razine %d\n",sig);
	pop_state();
	if(T_P != 0){
		PRINTF("Nastavlja se obrada prekida razine %d\n", T_P);
	} else {
		PRINTF("Nastavlja se izvodenje glavnog programa\n");
	}
	stanje();
}


void sklop_prihvat_prekida(int sig){
	if(obradaK_Z){
		PRINTF("SKLOP: promijenio se T_P, prosljeduje se prekid razine %d\n",sig);
		K_Z[sig-1] = 0;
		obradaK_Z = false;
		push_state(T_P);
		push_state(T_P);//registar
		switch(sig){
			case 3:
				obradi_sigint(sig);
				break;
			case 2:
				obradi_sigusr1(sig);
				break;
			case 1:
				obradi_sigterm(sig);
				break;
			default:
				PRINTF("ERROR!\n");
				exit(1);
		}
	}else{
		int prekid;
		switch(sig){
				case SIGINT:
					prekid = 3;
					break;
				case SIGUSR1:
					prekid = 2;
					break;
				case SIGTERM:
					prekid = 1;
					break;
				default:
					prekid = 0;
					break;
		}
		PRINTF("SKLOP: dogodio se prekid razine %d ", prekid);
		if((T_P < prekid)&&(K_Z[0] == 0 && K_Z[1] == 0 && K_Z[2] == 0)){
			printf("i prosljeduje se procesoru\n");
			K_Z[prekid-1] = 1;
			stanje();
			push_state(T_P);
			push_state(T_P);
			switch(sig){
				case SIGINT:
					obradi_sigint(prekid);
					break;
				case SIGUSR1:
					obradi_sigusr1(prekid);
					break;
				case SIGTERM:
					obradi_sigterm(prekid);
					break;
				default:
					PRINTF("Primljeni signal nema definiranu obradu\n");
					break;
			}
		} else if(T_P >= prekid){
			printf("ali on se pamti i ne prosljeduje procesoru\n");
			K_Z[prekid-1] = 1;
			stanje();
		} else if(T_P < prekid) {
			printf("i prosljeduje se procesoru\n");
			K_Z[prekid-1] = 1;
			stanje();
			push_state(T_P);
			push_state(T_P);
			if(T_P == 1){
				switch(prekid){
				case 2:
					obradi_sigusr1(prekid);
					break;
				case 3:
					obradi_sigint(prekid);
					break;
				}
			}else if(T_P = 2){
				obradi_sigint(prekid);
			}
		} else {
			printf("NEOBAVLJENI POSAO ZASTAVICA!!!\n\n");
		}
	}
}

void inicijalizacija()
{
	struct sigaction act;

	act.sa_handler = sklop_prihvat_prekida;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);

	act.sa_handler = sklop_prihvat_prekida;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);

	act.sa_handler = sklop_prihvat_prekida;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGTERM, &act, NULL);

	postavi_pocetno_vrijeme();
}


int main()
{
	inicijalizacija();

	PRINTF("G:Program s PID=%ld krenuo s radom.\n", (long) getpid());
	stanje();
	
	while(1){
		spavaj(1);
		for(int i = 2; i >= 0; i--){
			if(K_Z[i] != 0){
					obradaK_Z = true;
					sklop_prihvat_prekida(i+1);				
			}
		}
	}

	PRINTF("G: Kraj glavnog programa\n");

	return 0;
}