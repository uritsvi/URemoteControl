#ifndef __SCREEN__DELTA__PROCS__
#define __SCREEN__DELTA__PROCS__

#include <double_buffers.h>

#include "delta_struct.h"


typedef void(*OnScreenChanged)(
	DeltaStruct* delta_Struct, 
	DoubleBuffers* buffers);

typedef struct {
	OnScreenChanged on_screen_changed;
} ScreenDeltaProcs;

#endif