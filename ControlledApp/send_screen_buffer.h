#ifndef __SEND__SCREEN__BUFFER__
#define __SEND__SCREEN__BUFFER__

#include <delta_struct.h>


void init_send_screen_buffer();

void on_new_delta(
	DeltaStruct* buffer,
	DoubleBuffers* buffers);

void on_screen_delta_recived();

#endif