#ifndef __CONFIG__
#define __CONFIG__

#include "screen_delta_procs.h"

typedef struct {
	int target_width;
	int target_height;
	int screen_shot_bit_count;
	ScreenDeltaProcs* procs;

}Config;

void init_config(Config* config);

Config* get_config();

#endif