//
//  main.c
//  Exemplos
//
//  Created by Paulo Roberto Oliveira Valim on 23/03/17.
//  Copyright © 2017 Paulo Roberto Oliveira Valim. All rights reserved.
//
//Description
/*
 
Neste exemplo o mutex foi utilizado para garantir a exclusão mútua ao acessar a variável compartilhada. 

descrição de algumas funções utilizadas

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;: Define um mutex para garantir a exclusão mútua durante o acesso à variável 
		compartilhada.
pthread_cond_t condicao = PTHREAD_COND_INITIALIZER;: Define uma variável de condição que será usada para sincronizar as threads.
pthread_cond_wait(&condicao, &mutex);: Esta função bloqueia a thread atual até que a condição especificada seja satisfeita. 
		Ela libera o mutex enquanto espera e o bloqueia novamente antes de retornar.
pthread_cond_broadcast(&condicao);: Esta função emite um sinal para todas as threads que estão esperando na variável de condição.
pthread_mutex_lock(&mutex); e pthread_mutex_unlock(&mutex);: São usadas para bloquear e desbloquear o mutex antes e depois 
		de modificar a variável compartilhada, garantindo que apenas uma thread por vez possa acessá-la.
pthread_join(threads[i], NULL);: Aguarda a finalização de todas as threads antes de prosseguir.

exercício: modificar o código da thread reproduzindo as condições do ex05.c, porém agora com o uso do mutex. Executar 
           o código diversas vezes e observar o resultado.
		   - os resultados estão variando ou são sempre os mesmos? (sim/não e porque)
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 2

int variavel_compartilhada = 0; // Variável compartilhada entre as threads
int sinal = 0; // Variável de controle para sinalizar que as threads podem modificar a variável compartilhada
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para sincronização das threads
pthread_cond_t condicao = PTHREAD_COND_INITIALIZER; // Variável de condição para esperar o sinal

// Função executada pelas threads
void *funcao_thread(void *arg) {
    int *id = (int *)arg;
    
    // Bloqueia o mutex antes de acessar a variável compartilhada
    pthread_mutex_lock(&mutex);
    // Aguarda o sinal ser emitido
    while (!sinal) {
        pthread_cond_wait(&condicao, &mutex); // Aguarda o sinal e libera o mutex
    }
    printf("Thread %d - Recebeu sinal\n", *id);
    // Modifica a variável compartilhada
    variavel_compartilhada += 10;
    // Libera o mutex após modificar a variável compartilhada
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    int i;

    // Cria as threads
    for (i = 0; i < NUM_THREADS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, funcao_thread, &ids[i]);
    }

    // Emite o sinal para as threads
    printf("Sinal enviado\n");
    pthread_mutex_lock(&mutex); // Bloqueia o mutex antes de alterar a variável sinal
    sinal = 1; // Define o sinal como verdadeiro
    pthread_cond_broadcast(&condicao); // Emite o sinal para todas as threads
    pthread_mutex_unlock(&mutex); // Libera o mutex após alterar a variável sinal

    // Aguarda a finalização das threads
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Exibe o valor final da variável compartilhada
    printf("Variável compartilhada após as threads modificarem: %d\n", variavel_compartilhada);

    return 0;
}


