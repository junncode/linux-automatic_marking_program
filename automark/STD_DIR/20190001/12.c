#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

int length;
int buf[100];

void *ssu_thread_producer(void *arg) {
	int i;

	for (i = 1; i <= 300; i++) {
		pthread_mutex_lock(&mutex);

		if (length >= 100) { 
			length--;
			pthread_cond_signal(&cond2);
			pthread_cond_wait(&cond1, &mutex);
		}
		
		buf[length] = i;
		printf("p : buf[%d] = %d\n", length, buf[length]);
		length++;
		

		pthread_mutex_unlock(&mutex);
	}
	length--;

	pthread_cond_signal(&cond2);

	return NULL;
}

void *ssu_thread_consumer(void *arg) {
	int i;
	int sum = 0;

	for (i = 1; i <= 300; i++) {
		pthread_mutex_lock(&mutex);

		if (length == -1) {
			length++;
			pthread_cond_signal(&cond1);
			pthread_cond_wait(&cond2, &mutex);
		}
		
		printf("    s : buf[%d] = %d\n", length, buf[length]);
		sum += buf[length];
		length--;
		
		pthread_mutex_unlock(&mutex);
	}

	printf("%d\n", sum);
	return NULL;
}

int main(void) {
	pthread_t producer_tid, consumer_tid;

	pthread_create(&producer_tid, NULL, ssu_thread_producer, NULL);
	pthread_create(&consumer_tid, NULL, ssu_thread_consumer, NULL);

	pthread_join(producer_tid, NULL);
	pthread_join(consumer_tid, NULL);

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond1);
	pthread_cond_destroy(&cond2);
	exit(0);
}
