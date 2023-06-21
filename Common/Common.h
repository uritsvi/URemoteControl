#ifndef __COMMON__
#define __COMMON__

#define APP_NAME L"URemoteControl"

#define BITE_MAX_VALUE 256
#define DEFAULT_BUFFER_SIZE 256

#define CONTROL_CHANEL_CLIENT_TYPE 0
#define SYNC_SCREEN_BUFFER_CLIENT_TYPE 1
#define SYNC_IO_CLIENT_TYPE 2

#define CLIENT_TARGET_READ 0
#define CLIENT_TARGET_WRITE 1

#define CONTROLED_CLIENT_INDEX 0
#define CONTROLER_CLIENT_INDEX 1

#define ALL_CLIENTS_CONNECTED_MSG 0
#define DATA_RECIVED_MSG 1

#define MAX_OBJECTS 10
#define UREMOTE_CONTROL_MAX_MONITORS 10
	
// INI FILE KEYS
#define INI_FILE_NAME_EXTANSION "\\config.ini"

#define INI_CONFIG_SECTION_NAME "config"

#define INI_TARGET_WIDTH_KEY "target_width"
#define INI_TARGET_HEIGHT_KEY "target_height"
#define INI_TARGET_BITCOUNT_KEY "target_bit_count"

#define INI_COMPRESSION_LEVEL "compression_level"

#define INI_MAX_SEND_BUFFER "max_send_buffer"

#define INI_NUM_OF_DELTA_PARTS "num_of_delta_parts"

#define INI_CAPTURE_FULL_SCREEN_INTERVALT "capture_full_screen_interval"

#endif