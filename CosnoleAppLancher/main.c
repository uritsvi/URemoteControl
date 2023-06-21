#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <platfrom_memory_utils.h>
#include <processes.h>
#include <Common.h>
#include <error.h>
#include <platform.h>

#include <windows.h>

#pragma comment(lib, "Platform.lib")

#define NUM_OF_ARGS 2

#define ADDRESS_INDEX 0
#define PORT_INDEX 1

#define DEFAULT_SERVER_PORT "80"

bool _try_run_app(
	char* app_type, 
	char* args[]) {

	char process_path[DEFAULT_BUFFER_SIZE] = {0};

	process_path[0] = '"';
	get_runing_dir(process_path + 1);

	if (strlen(process_path) >= DEFAULT_BUFFER_SIZE - 2) {
		platform_exit_with_error("Failed to get running dir");
	}

	bool is_valid_app = false;
	if (strcmp(app_type, "1") == 0) {
		is_valid_app = true;

		const char* app_name_extension = "\\ControllerApp.exe";

		strcat_s(
			process_path,
			DEFAULT_BUFFER_SIZE,
			app_name_extension);
	}
	else if (strcmp(app_type, "2") == 0) {
		const char* app_name = "\\ControlledApp.exe";

		is_valid_app = true;
		strcat_s(
			process_path,
			DEFAULT_BUFFER_SIZE,
			app_name);
	}

	int len = strlen(process_path);
	process_path[len] = '"';
	process_path[len + 1] = '\0';

	if (is_valid_app) {
		Process process;
		bool res = create_process(
			process_path,
			NUM_OF_ARGS,
			args,
			&process);

		if (!res) {
			platform_exit_with_error("Failed to create process\n");
			return false;
		}
	}


	return is_valid_app;

}

int main(int argc, char* argv[]) {

	if (argc > 1) {
		char* args[NUM_OF_ARGS];
		args[0] = argv[2];
		args[1] = argv[3];

		_try_run_app(argv[1], args);

		return;
	}

	while (true) {

		printf("Enter server address\n");

		char* args[NUM_OF_ARGS];

		for (int i = 0; i < NUM_OF_ARGS; i++) {
			args[i] = safe_malloc(DEFAULT_BUFFER_SIZE);
		}

		fgets(args[ADDRESS_INDEX], DEFAULT_BUFFER_SIZE, stdin);
		args[ADDRESS_INDEX][strlen(args[ADDRESS_INDEX]) - 1] = 0;

		while (true) {
			printf("Enter app type, 1 : controller , 2 : controlled\n");

			char app_type[DEFAULT_BUFFER_SIZE];
			fgets(app_type, DEFAULT_BUFFER_SIZE, stdin);
			app_type[strlen(app_type) - 1] = 0;

			strcpy_s(args[PORT_INDEX], 
				     DEFAULT_BUFFER_SIZE,
					 DEFAULT_SERVER_PORT);

			bool is_valid_app =
				_try_run_app(
					app_type, 
					args);

			if (!is_valid_app) {
				printf("App type does not exist\n");
				continue;
			}
		}
	}
}