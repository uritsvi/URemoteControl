
#ifdef _WIN32

#include <Windows.h>
#include <psapi.h>

#include <Common.h>
#include <stdio.h>

#include "app_instances.h"

#define PROCESSES_BUFFER_SIZE 2048
#define BUFFER_SIZE 512


void get_exe_name(
	char* src, 
	char* dest,
	int buffer_size) {


	char* last_slash = strrchr(
		src,
		'\\');

	last_slash++;

	int size = (unsigned int)(src + buffer_size) - (unsigned int)(last_slash);


	memcpy(
		dest,
		last_slash,
		size);

	dest[size] = '\0';

}

bool is_only_app_instance() {

	
	char exe_name[BUFFER_SIZE];
	
	GetModuleFileNameA(
		GetModuleHandleA(NULL), 
		exe_name, 
		BUFFER_SIZE);

	get_exe_name(
		exe_name,
		exe_name,
		strlen(exe_name));


	int processes[PROCESSES_BUFFER_SIZE];
	int num_of_processes;
	
	EnumProcesses(
		processes, 
		PROCESSES_BUFFER_SIZE, 
		&num_of_processes);

	for (int i = 0; i < num_of_processes; i++) {
		if (processes[i] == GetCurrentProcessId()) {
			continue;
		}

		HANDLE process_handle = OpenProcess(
			PROCESS_QUERY_LIMITED_INFORMATION,
			FALSE, 
			processes[i]);

		if (process_handle == NULL) {
			continue;
		}

		int buffer_size = BUFFER_SIZE;
		char current_processes_name[BUFFER_SIZE];
		BOOL res = QueryFullProcessImageNameA(
			process_handle, 
			0,
			current_processes_name, 
			&buffer_size);

		if (!res) {
			continue;
		}


		get_exe_name(
			current_processes_name, 
			current_processes_name, 
			buffer_size);


		if (strcmp(
			current_processes_name, 
			exe_name) == 0) {

			return false;
		}
	}

	return true;
}

#endif