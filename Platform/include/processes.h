#ifndef __PROCESSES__
#define __PROCESSES__

#include <stdbool.h>

typedef int Process;


bool create_process(const char* name,
					int num_of_args,
					char* args[],
					Process* out);


#endif