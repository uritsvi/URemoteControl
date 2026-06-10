#ifndef __RECEIVE__SCREEN__BUFFER__
#define __RECEIVE__SCREEN__BUFFER__

#include <platform_threads.h>

void init_receive_screen_buffer();

void receive_screen_buffer(
	char* out,
	Event work_done);

#endif