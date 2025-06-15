#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstddef>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_SIZE 8

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
sem_t s;
int readCount = 0;

typedef struct{
	int data[BUFFER_SIZE];
	int read_ptr;
	int write_ptr;
	bool full;
}RingBuffer ;

// Helper: advance pointer with wraparound
static int advace(int ptr){
	return (ptr +1) % BUFFER_SIZE;
}

void ring_init(RingBuffer *rb){
	rb->read_ptr =0;
	rb->write_ptr =0;
	rb->full = false;
}

//Push: overwrite if full
void ring_push(RingBuffer *rb, int value){
	rb->data[rb->write_ptr] = value;
	int next_write = advace(rb->write_ptr);
	if (rb->full){
		// Overwrite oldest value: move read_ptr forward
		rb->read_ptr = advace(rb->write_ptr);
	}

	rb->write_ptr = next_write;
	rb->full = (rb->write_ptr == rb->read_ptr);
}

//Pop: return false if empty
bool ring_pop(RingBuffer *rb, int *value){
	if (rb->read_ptr == rb->write_ptr && !rb->full){
		return false; //Empty
	}
	*value = rb->data[rb->read_ptr];
	rb->full = false;
	return true;
}

// Status helpers
bool ring_is_empty(RingBuffer *rb){
	return (rb->read_ptr == rb->write_ptr)&&!rb->full;
}

bool ring_is_full(RingBuffer* rb){
	return rb->full;
}

int ring_size(RingBuffer *rb){
	if (rb->full) return BUFFER_SIZE;
	if (rb->write_ptr >= rb->read_ptr){
		return rb->write_ptr - rb->read_ptr;
	}
	return  BUFFER_SIZE - rb->read_ptr + rb->write_ptr;
}

void* threadWriter(void* arg) {
    RingBuffer* rb = (RingBuffer*)arg;
    for (int i = 0; i < 10; ++i) {
        int temp = rand() % 100;

        pthread_mutex_lock(&m);
        ring_push(rb, temp);
        printf("Pushed: %d\n", temp);
        pthread_mutex_unlock(&m);

        sleep(1);
    }
    return NULL;
}

void* threadReader(void* arg) {
    RingBuffer* rb = (RingBuffer*) arg;
    int val;

    for (int i = 0; i < 10; ++i) {
        pthread_mutex_lock(&m);
        readCount++;
        if (readCount == 1) {
            sem_wait(&s);  // down für writer
        }
        pthread_mutex_unlock(&m);

        // Lesen aus RingBuffer
        if (ring_pop(rb, &val)) {
            printf("Popped: %d(size: %d)\n", val, ring_size(rb));
        }

        pthread_mutex_lock(&m);
        readCount--;
        if (readCount == 0) {
            sem_post(&s);  // up für Writer
        }
        pthread_mutex_unlock(&m);

        sleep(1);  // optionaler Abstand fürs Simulieren
    }

    return NULL;
}

int main(){
	srand(time(NULL));

	pthread_t writers[5];
	pthread_t readers[5];
	RingBuffer rb;
	ring_init(&rb);

	for (int i=0; i<5; i++){
		pthread_create(&writers[i], NULL, threadWriter, &rb);
	}

	for (int i=0; i<5; i++){
		pthread_create(&readers[i], NULL, threadReader, &rb);
	}

	//threads wieder zusammenführen
	for (int i = 0; i < 3; i++) {
		pthread_join(writers[i], NULL);
	}
	for (int i = 0; i < 2; i++) {
		pthread_join(readers[i], NULL);
	}

	sem_destroy(&s);
	
	return 0;
}

