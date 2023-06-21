#ifndef __DOUBLE__BUFFERS__
#define __DOUBLE__BUFFERS__


typedef struct {
	char* front;
	char* back;

	int back_buffer_size;
	int front_buffer_size;
} DoubleBuffers;


void create_double_buffers(
	int size, 
	DoubleBuffers* out);

void create_double_buffers_from_existing(
	char* front,
	char* back,
	DoubleBuffers* out);

void switch_buffers(DoubleBuffers* buffers);

#endif