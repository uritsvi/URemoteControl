#ifndef __SCREEN__DELTA__
#define __SCREEN__DELTA__

#include "screen_delta_procs.h"
#include <program_config.h>

void init_screen_delta(ProgramConfig* program_config,
					   ScreenDeltaProcs* proces);

void set_current_monirot(int index);

void shut_down_delta();


#endif