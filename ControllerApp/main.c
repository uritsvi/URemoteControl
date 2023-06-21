#include <memory.h>

#include <program_config.h>
#include <Common.h>
#include <platfrom_memory_utils.h>
#include <read_ini.h>
#include <entry_point.h>
#include <app_instances.h>
#include <error.h>
#include <network.h>

#include "receive_screen_buffer.h"
#include "app.h"


#pragma comment(lib, "Platform.lib")
#pragma comment(lib, "Helpers.lib")

#define MUTEX_APP_NAME "Controller"

void init() {
	ProgramConfig* config = get_program_config();
	bool res = init_networking(config->max_send_buffer);
	if (!res) {
		platform_exit_with_error("Failed to init networking\n");
	}
}

void init_confg(
	const char* address,
	const char* port) {
	ProgramConfig* config = safe_malloc(sizeof(ProgramConfig));

	config->server_address = safe_malloc(strlen(address) + 1);
	config->server_port = safe_malloc(strlen(port) + 1);
	config->client_index = CONTROLER_CLIENT_INDEX;
	strcpy_s(config->server_address, strlen(address) + 1, address);
	strcpy_s(config->server_port, strlen(port) + 1, port);

	read_ini(config);

	init_program_config(config);

}

void entry_point(int argc, char* argv[]) {
	if (!is_only_app_instance()) {
		return;
	}

	init_confg(
		argv[1],
		argv[2]);

	init();


	run_app();
}