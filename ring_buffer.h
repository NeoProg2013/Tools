//  ***************************************************************************
/// @file    ring_buffer.h
/// @author  NeoProg
/// @brief   Ring buffer
//  ***************************************************************************
#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_
#include <stdint.h>
#include <stdbool.h>

// Change this value for increase or decrease ring buffer size
#define RING_BUFFER_SIZE             (5)

// Define new ID for create more buffers
typedef enum {
	RING_BUFFER_1,
	RING_BUFFERS_COUNT
} ring_buffer_id;

extern void ring_buffer_init(ring_buffer_id buffer_id);
extern void ring_buffer_push(ring_buffer_id buffer_id, uint8_t data);
extern bool ring_buffer_pop(ring_buffer_id buffer_id, uint8_t* data);
extern bool ring_buffer_is_empty(ring_buffer_id buffer_id);
extern void ring_buffer_clear(ring_buffer_id buffer_id);

//extern void ring_buffer_print(ring_buffer_id buffer_id);


#endif // _RING_BUFFER_H_
