#include "program_config.h"

static ProgramConfig* g_config;

void init_program_config(ProgramConfig* config) {
	g_config = config;
}
ProgramConfig* get_program_config() {
	return g_config;
}

void build_target_rect(
	ProgramConfig* config,
	Rect* dest) {
	
	
	dest->left = 0;
	dest->top = 0;
	dest->right = config->target_width;
	dest->bottom = config->target_height;
}