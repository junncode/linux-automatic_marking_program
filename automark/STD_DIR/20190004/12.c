#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int length;
int buf[100];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

void *ssu_thread_producer(void* arg) {
	int i;
	
	for (i = 1; i <= 300; i++) {
		pthread_mutex_lock(&mutex);

		buf[length++] = i;
		pthread_cond_signal(&cond2);

		if (length == 100) {
			pthread_cond_wait(&cond1, &mutex);
		}

		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

void *ssu_thread_consumer(void* arg) {
	int i;
	int sum = 0;

	for (i = 1; i <= 300; i++) {
		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&cond1);

		if (length == 0) {
			pthread_cond_wait(&cond2, &mutex);
		}

		sum += buf[--length];
		pthread_mutex_unlock(&mutex);
	}

	printf("%d\n", sum);
	return NULL;
}

void ssu_signal(int signo) {
	printf("SIGUSR1 catched!!\n");
}

int main(void)
{
	pthread_t producer_tid, consumer_tid;

	pthread_create(&producer_tid, NULL, ssu_thread_producer, NULL);
	pthread_create(&consumer_tid, NULL, ssu_thread_consumer, NULL);

	pthread_join(producer_tid, NULL);
	pthread_join(consumer_tid, NULL);

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond1);
	pthread_cond_destroy(&cond2);
	exit(0);

