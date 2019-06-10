#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 100

void * numPrimos(void * inicial){
	long int tid;
	tid = (long int) inicial;

	for(long int valor = tid; valor < tid+1000; valor++){
		long int d = 2;
		while(valor%d != 0 && d*d <= valor) d++;
		if(valor%d != 0) printf("%ld\n", valor);
	}
	pthread_exit(NULL);
}

int main(){
	pthread_t threads[NUM_THREADS];

	for(long int cont = 0; cont < 100; cont++){
		printf("Criando thread para intervalo %ld - %ld\n", cont*1000, cont*1000+999);
		//endereço da thread, Prioridade da thread, nome da função e argumento da função
		pthread_create(&threads[cont], NULL, numPrimos, (void*)(cont*1000) );
	}
	pthread_exit(NULL);
	return 0;
}