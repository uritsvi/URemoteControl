#include "config.h"

static Config* g_Config;

void init_config(Config* config) {
	g_Config = config;
}

Config* get_config() {
	return g_Config;
}