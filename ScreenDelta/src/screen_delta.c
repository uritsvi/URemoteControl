#include <platfrom_memory_utils.h>

#include "screen_delta.h"
#include "delta_finder.h"


void init_screen_delta(ProgramConfig* program_config,
					   ScreenDeltaProcs* procs) {

	init_delta_finder(
		program_config, 
		procs);
}

void set_current_monirot(int index) {
	delta_finder_set_current_monitor(index);
}

void shut_down_delta() {
}