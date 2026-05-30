#ifndef __PROGRAM__CONFIG__
#define __PROGRAM__CONFIG__

#include <Common.h>
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

	bool use_tls;
	char tls_server_name[DEFAULT_BUFFER_SIZE];
	char tls_ca_cert_path[DEFAULT_BUFFER_SIZE];
	bool allow_remote_keyboard_control;
	bool allow_remote_mouse_control;

	/* Shared passphrase for end-to-end encryption. Empty disables it. */
	char e2e_key[DEFAULT_BUFFER_SIZE];

	/*
	 * Debug mode: the controller does not capture/forward input and does not
	 * trap the cursor, the controlled side does not apply remote input, and the
	 * controller window opens in (non full screen) windowed mode. This keeps
	 * the local mouse and keyboard usable while testing.
	 */
	bool debug_mode;

} ProgramConfig;


void init_program_config(ProgramConfig* config);
ProgramConfig* get_program_config();

void build_target_rect(
	ProgramConfig* config, 
	Rect* dest);

#endif