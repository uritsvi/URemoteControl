#ifndef __DELTA__FINDER__
#define __DELTA__FINDER__

#include <program_config.h>
#include <screen_delta_procs.h>

void init_delta_finder(
	ProgramConfig* program_config, 
	ScreenDeltaProcs* procs);


void delta_finder_set_current_monitor(int index);

#endif