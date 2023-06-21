#ifdef _WIN32 

#include <Windows.h>
#include <platfrom_memory_utils.h>

#include <Common.h>

#include "processes.h"


typedef struct{
	HANDLE process;
	Process out;
}  WIN32Process;

static struct WIN32Process* g_Processes[MAX_OBJECTS];
static int g_NextprocessIndex = 0;

static struct WIN32Process* 
	_create_WIN32Process(PROCESS_INFORMATION* info, 
						 Process* out) {
	
	WIN32Process* outProcess = 
		safe_malloc(sizeof(WIN32Process));

	outProcess->process = info->hProcess;

	*out = g_NextprocessIndex++;
	outProcess->out = *out;

	return outProcess;
}


bool create_process(const char* name,
				    int num_of_args,
					char* args[],
					Process* out) {

	STARTUPINFO start_up_info;
	ZeroMemory(&start_up_info, sizeof(start_up_info));

	PROCESS_INFORMATION process_info;
	ZeroMemory(&process_info, sizeof(process_info));

	start_up_info.cb = sizeof(start_up_info);

	/*
	* allocates space for all the args + the process name
	* for each parameters allocates extra space for the 0x20 char
	*/

	char* cmd = safe_malloc((num_of_args + 1) * (DEFAULT_BUFFER_SIZE + 1));

	int i = 0;
	char* current_arg = name;
	int last_index = 0;
	do{
		strcpy_s(cmd + last_index, DEFAULT_BUFFER_SIZE, current_arg);
		last_index += (int)strlen(current_arg);

		cmd[last_index] = 0x20;

		current_arg = args[i++];
		last_index++;
	} while (i <= num_of_args);

	cmd[last_index] = '\0';

	

	BOOL res = CreateProcessA(NULL,
				   cmd, 
				   NULL, 
				   NULL, 
				   FALSE, 
				   NORMAL_PRIORITY_CLASS, 
				   NULL, 
			       NULL, 
				   (LPSTARTUPINFO)&start_up_info,
				   &process_info);


	if (!res) {
		return false;
	}

	struct WIN32Process* process =
		_create_WIN32Process(&process_info, out);

	g_Processes[*out] = process;

	
	return true;
}


#endif