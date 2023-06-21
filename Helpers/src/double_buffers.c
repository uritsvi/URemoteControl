#include <Common.h>
#include <platfrom_memory_utils.h>

#include "double_buffers.h"

void create_double_buffers(
	int size,
	DoubleBuffers* out) {

	out->front = safe_malloc(size);
	out->back = safe_malloc(size);

	out->front_buffer_size = size;
	out->back_buffer_size = size;
}

void create_double_buffers_from_existing(
	char* front,
	char* back,
	DoubleBuffers* out) {

	out->front = front;
	out->back = back;
}

void switch_buffers(DoubleBuffers* buffers) {
	char* last_front = buffers->front;
	buffers->front = buffers->back;
	buffers->back = last_front;
}