#ifndef __RECEIVE__INPUT__
#define __RECEIVE__INPUT__

#include <input.h>
#include <platform_threads.h>

void init_receive_io();

void receive_io(InputStruct* buffer);

#endif