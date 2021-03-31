//  ***************************************************************************
/// @file    ring_buffer.c
/// @author  NeoProg
//  ***************************************************************************
#include "ring_buffer.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t data;   // Node data
	void*   next;   // Pointer to next node
} node_t;

typedef struct {
	node_t   nodes[RING_BUFFER_SIZE];   // Node list
	node_t*  head;						// Pointer to head of buffer
	node_t*  tail;						// Pointer to tail of buffer
} ring_buffer_t;


static ring_buffer_t ring_buffer[RING_BUFFERS_COUNT] = {0};


//  ***************************************************************************
/// @brief  Ring buffer initialization
/// @param  buffer_id: ring buffer id
/// @return none
//  ***************************************************************************
void ring_buffer_init(ring_buffer_id buffer_id) {
	ring_buffer_t* buffer = &ring_buffer[buffer_id];

	// Make nodes loop: [0]->[1]->[...]->[N]->[0]
	for (uint32_t i = 0; i < RING_BUFFER_SIZE - 1; ++i) {
		buffer->nodes[i].data = 0;
		buffer->nodes[i].next = &buffer->nodes[i + 1];
	}
	buffer->nodes[RING_BUFFER_SIZE - 1].data = 0;
	buffer->nodes[RING_BUFFER_SIZE - 1].next = &buffer->nodes[0];

	// Initialization head and tail
	buffer->head = NULL;
	buffer->tail = NULL;
}

//  ***************************************************************************
/// @brief  Push data to ring buffer
/// @param  buffer_id: ring buffer id
/// @param  data: data for enqueue
//  ***************************************************************************
void ring_buffer_push(ring_buffer_id buffer_id, uint8_t data) {
	ring_buffer_t* buffer = &ring_buffer[buffer_id];

	if (buffer->tail != NULL && ring_buffer->tail->next == buffer->head) { // Buffer is overflow
		buffer->head = buffer->head->next;
		buffer->tail = buffer->tail->next;
		buffer->tail->data = data;
	}
	else {
		if (buffer->head == NULL) { // Insert first item as head of queue
			buffer->head = &buffer->nodes[0];
			buffer->head->data = data;
		}
		else if (buffer->tail == NULL) { // Insert second as tail of queue
			buffer->tail = (node_t*)buffer->head->next;
			buffer->tail->data = data;
		}
		else {
			buffer->tail = (node_t*)buffer->tail->next;
			buffer->tail->data = data;
		}
	}
}

//  ***************************************************************************
/// @brief  Pop data from ring buffer
/// @param  buffer_id: ring buffer id
/// @param  data: buffer for data
/// @return true - pop success, false - ring buffer is empty
//  ***************************************************************************
bool ring_buffer_pop(ring_buffer_id buffer_id, uint8_t* data) {
	ring_buffer_t* buffer = &ring_buffer[buffer_id];

	if (buffer->head == NULL) {
		return false; // Queue is empty
	}

	// Read data from queue and clear this node
	*data = buffer->head->data;
	buffer->head->data = 0;
	buffer->head = (node_t*)buffer->head->next;

	if (buffer->head == buffer->tail) {
		buffer->tail = NULL; // Remaining last item as head - remove tail
	}
	else if (buffer->tail == NULL) {
		buffer->head = NULL; // We read last item - remove head, ring buffer now is empty
	}
	return true;
}

//  ***************************************************************************
/// @brief  Check ring buffer empty
/// @param  buffer_id: ring buffer id
/// @return true - queue is empty, false - otherwise
//  ***************************************************************************
bool ring_buffer_is_empty(ring_buffer_id buffer_id) {
	return ring_buffer[buffer_id].head == NULL;
}

//  ***************************************************************************
/// @brief  Clear ring buffer
/// @param  buffer_id: ring buffer id
/// @return none
//  ***************************************************************************
void ring_buffer_clear(ring_buffer_id buffer_id) {
	ring_buffer_init(buffer_id);
}


/*#include <stdio.h>
void ring_buffer_print(ring_buffer_id buffer_id) {
	ring_buffer_t* buffer = &ring_buffer[buffer_id];

	for (uint32_t i = 0; i < RING_BUFFER_SIZE; ++i) {
		if (ring_buffer[buffer_id].head == &buffer->nodes[i]) {
			printf("[%d] ", buffer->nodes[i].data);
		}
		else if (ring_buffer[buffer_id].tail == &buffer->nodes[i]) {
			printf("{%d} ", buffer->nodes[i].data);
		}
		else {
			printf("%d ", buffer->nodes[i].data);
		}
	}
	printf("\n");
}*/