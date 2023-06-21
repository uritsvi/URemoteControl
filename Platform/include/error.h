#ifndef __ERROR__
#define __ERROR__

typedef void(*ShutDown)();

void init_error(ShutDown shutdown);
void platform_exit_with_error(const char* error_msg);

#endif