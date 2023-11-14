#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdbool.h>

// queue node
typedef struct QueueNode {
    void *data;
    struct QueueNode* next;
} QueueNode;

QueueNode* first;
QueueNode* last;
mtx_t qlock;
cnd_t notEmpty;
size_t elements_amount;
size_t elements_waiting;
size_t elements_visited;

// Function to create an empty queue
void initQueue() {
    first = NULL;
    last = NULL;
    elements_amount = 0;
}

// check if the queue is empty
int isEmpty() {
    return (first == NULL);
}

// Add an element to as the last element of the queue
void enqueue_not_locked(void *data) {
		//update elements_amount counter:
		elements_amount++;
		
    // Create a new queue node
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->data = data;
    newNode->next = NULL;

    // If the queue is empty, set the new node as both first and last elemet
    if (isEmpty()) {
        first = newNode;
        last = newNode;
    } else {
        // Otherwise, add the new node as the last element
        last->next = newNode;
        last = newNode;
    }
}

void enqueue(void *data) {
	//lock the enqueue function so that diffrent threds won't enqueue to the same location
	mtx_lock(&qlock);
	enqueue_not_locked(data);
	//send signal for dequeue waiting when queue is empty
	cnd_signal(&notEmpty);
	mtx_unlock(&qlock);
}

// Remove the first element of the queue
void *dequeue_not_locked() {
		//update elements_amount and elements_visited counters:
		elements_amount = elements_amount-1;
		elements_visited++;
		
    // Store the fisrt node in the queue and move the first pointer to the next node
    QueueNode* temp = first;
    void *data = temp->data;
    first = first->next;

    // If the fisrt element becomes NULL, set the last element to NULL as well
    if (first == NULL) {
        last = NULL;
    }

    free(temp); // Free the memory allocated for the previous first node
    return data; // Return the data of the removed node
}

void *dequeue() {
		//lock the enqueue function so that diffrent threds won't dequeue the same item
		//this is the same lock for enqueue so that enqueue and dequeue won't happen at the same time (this can cause problems like dequeue returning wrong element)
		mtx_lock(&qlock);
		while (isEmpty()) {
				//wait until the queue wob't be empty
				elements_waiting++; //new thread starts waiting
        cnd_wait(&notEmpty,&qlock);
        elements_waiting = elements_waiting-1; //thread finished waiting
    }
    //then dequeue the element
    void *data = dequeue_not_locked();
    mtx_unlock(&qlock);
    return data;

}

// Free the memory allocated for the queue
void destroyQueue() {
    while (!isEmpty()) {
        dequeue();
    }
    free(first);
    free(last);
}

bool tryDequeue(void **element_ptr){
	//lock so that the empty checking will be consistent with the deque function
	mtx_lock(&qlock);
	if (isEmpty()){
		mtx_unlock(&qlock);
		return false;
	}
	else{
		void *element = dequeue_not_locked();
		element_ptr = &element;
		mtx_unlock(&qlock);
		return true;
	}
}

size_t size(void){
	return elements_amount;
}

size_t waiting(void){
	return elements_waiting;
}

size_t visited(void){
	return elements_visited;
}
