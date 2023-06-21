#ifndef __PROGRAM__CONFIG__
#define __PROGRAM__CONFIG__

#include <rect.h>

typedef struct {

	const char* server_address;
	const char* server_port;
	int client_index;

	int target_width;
	int target_height;
	int target_bit_count;

	int window_width;
	int window_height;

	int compression_level;

	int max_send_buffer;

	int num_of_delta_parts;

	int capture_full_screen_interval;

} ProgramConfig;


void init_program_config(ProgramConfig* config);
ProgramConfig* get_program_config();

void build_target_rect(
	ProgramConfig* config, 
	Rect* dest);

#endif