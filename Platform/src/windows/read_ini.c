#ifdef _WIN32

#include <Windows.h>
#include <Shlwapi.h>

#include <string.h>

#include <Common.h>
#include <platform.h>
#include <error.h>

#include <read_ini.h>

#pragma comment(lib, "Shlwapi.lib")

void _error() {
	platform_exit_with_error("Failed to read data to ini file\n");
}

static bool _parse_ini_bool(const char* value) {
	return (value[0] == '1' || _stricmp(value, "true") == 0);
}

void read_ini(ProgramConfig* config){
	char ini_file_path[DEFAULT_BUFFER_SIZE];
	char buffer[DEFAULT_BUFFER_SIZE];

	get_runing_dir(ini_file_path);
	strcat_s(
		ini_file_path, 
		DEFAULT_BUFFER_SIZE, 
		INI_FILE_NAME_EXTANSION);

	DWORD res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME, 
		INI_TARGET_WIDTH_KEY, 
		NULL, 
		buffer, 
		DEFAULT_BUFFER_SIZE, 
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->target_width = 
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_TARGET_HEIGHT_KEY,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->target_height =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_TARGET_BITCOUNT_KEY,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->target_bit_count =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_TARGET_WIDTH_KEY,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->window_width =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_TARGET_HEIGHT_KEY,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->window_height =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_COMPRESSION_LEVEL,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->compression_level =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_MAX_SEND_BUFFER,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->max_send_buffer =
		StrToIntA(buffer);


	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_NUM_OF_DELTA_PARTS,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->num_of_delta_parts =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_CAPTURE_FULL_SCREEN_INTERVALT,
		NULL,
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	if (res == 0) {
		_error();
	}

	config->capture_full_screen_interval =
		StrToIntA(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_USE_TLS,
		"0",
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	config->use_tls = _parse_ini_bool(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_TLS_SERVER_NAME,
		"",
		config->tls_server_name,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_TLS_CA_CERT_PATH,
		"",
		config->tls_ca_cert_path,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_ALLOW_REMOTE_KEYBOARD_CONTROL,
		"0",
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);
	config->allow_remote_keyboard_control = _parse_ini_bool(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_ALLOW_REMOTE_MOUSE_CONTROL,
		"0",
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);
	config->allow_remote_mouse_control = _parse_ini_bool(buffer);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_E2E_KEY,
		"",
		config->e2e_key,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);

	res = GetPrivateProfileStringA(
		INI_CONFIG_SECTION_NAME,
		INI_DEBUG_MODE,
		"0",
		buffer,
		DEFAULT_BUFFER_SIZE,
		ini_file_path);
	config->debug_mode = _parse_ini_bool(buffer);

}

#endif