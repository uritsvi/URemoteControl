#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <Common.h>

#include <zlib.h>

#include <platform_threads.h>
#include <error.h>
#include <network.h>
#include <program_config.h>
#include <connect_to_server.h>
#include <double_buffers.h>
#include <platfrom_memory_utils.h>
#include <platform.h>
#include <screen_delta.h>

#include "send_screen_buffer.h"


static Event g_WorkToDo;

static Mutex g_Lock;
static Mutex g_SendLock;

static Socket g_Socket;

static DeltaStruct* g_CurrentBuffer;

static bool g_ServerRecivedData;

static int g_DeltaRecivedCound;

static char* g_Compressed;
static int g_CompressedBufferSize;

/*
 * Contiguous [DeltaPacketInfo][compressed] scratch buffer. The screen payload
 * has to be a single contiguous block so it can be sealed as one unit before it
 * is handed to the relay server (see send_encrypted_data).
 */
static char* g_PacketScratch;

void _send_delta_packet(
	DeltaPacket delta,
	int buffer_size) {

	int total = sizeof(DeltaPacketInfo) + buffer_size;

	memcpy(
		g_PacketScratch,
		&delta.info,
		sizeof(DeltaPacketInfo));

	memcpy(
		g_PacketScratch + sizeof(DeltaPacketInfo),
		g_Compressed,
		buffer_size);

	send_encrypted_data(
		g_Socket,
		g_PacketScratch,
		total);

	return;
}

void _handle_send_delta(DeltaStruct* delta_struct) {
	g_DeltaRecivedCound = delta_struct->num_of_rects;
	ProgramConfig* config = get_program_config();

	for (int i = 0; i < delta_struct->num_of_rects; i++) {
		
		
		int compressed_size = g_CompressedBufferSize;
		int res = compress2(
			g_Compressed,
			&compressed_size,
			delta_struct->buffer + delta_struct->rects[i].offset,
			delta_struct->rects[i].buffer_size,
			config->compression_level);
		
		if (res != 0) {
			platform_exit_with_error("Failed to compress buffer");
		}

		DeltaPacket delta;
		DeltaPacketInfo info;

		info.rect = delta_struct->rects[i].rect;
		info.compressed_size = compressed_size;

		delta.info = info;
		delta.buffer_ptr = g_Compressed;

		_send_delta_packet(
			delta, 
			info.compressed_size);
	}
}

void _send_screen_buffer_proc() {
	while (true) {
		
		wait_event(g_WorkToDo);
		lock_mutex(g_SendLock);

		
		_handle_send_delta(g_CurrentBuffer);
		

		reset_event(g_WorkToDo);
		release_mutex(g_SendLock);
		g_ServerRecivedData = true;
		
	}
}

void init_send_screen_buffer(){

	ProgramConfig* config = 
		get_program_config();

	int raw_frame_size =
		config->target_width *
		config->target_height *
		config->target_bit_count / 8;

	/*
	 * zlib output can be slightly LARGER than its input when the screen shows
	 * incompressible content (video, noise, photos). Size the compression
	 * destination with compressBound so compress2 never fails with Z_BUF_ERROR
	 * on a full-screen frame.
	 */
	g_CompressedBufferSize = compressBound(raw_frame_size);


	g_Compressed = safe_malloc(g_CompressedBufferSize);

	g_PacketScratch = safe_malloc(
		sizeof(DeltaPacketInfo) + g_CompressedBufferSize);

	bool res =
		connect_to_server(
			config->server_address,
			config->server_port,
			config->max_send_buffer,
			SYNC_SCREEN_BUFFER_CLIENT_TYPE,
			CLIENT_TARGET_WRITE,
			config->client_index,
			true,
			&g_Socket);


	if (!res) {
		platform_exit_with_error("Failed to connect to server send screen buffer client\n");
		return;
	}

	res = create_event(&g_WorkToDo);
	if (!res) {
		platform_exit_with_error("Failed to create event for send screen buffer\n");
		return;
	}

	res = create_mutex(&g_Lock);
	if (!res) {
		platform_exit_with_error("Failed to create mutex for send screen buffer\n");
		return;
	}
	res = create_mutex(&g_SendLock);
	if (!res) {
		platform_exit_with_error("Failed to create mutex for send screen buffer\n");
		return;
	}
	
	Thread thread;
	res = create_thread(
		_send_screen_buffer_proc, 
		NULL, 
		&thread);
	
	if (!res) {
		platform_exit_with_error("Failed to create thread for send the screen buffer\n");
		return;
	}

	g_ServerRecivedData = true;
}

void on_new_delta(
	DeltaStruct* buffer,
	DoubleBuffers* buffers) {

	
	bool res = try_lock(g_SendLock);
	if (!res || !g_ServerRecivedData) {
		return;
	}

	g_ServerRecivedData = false;

	g_CurrentBuffer = buffer;
	switch_buffers(buffers);

	set_event(g_WorkToDo);

	release_mutex(g_SendLock);
}

void on_screen_delta_recived() {
	lock_mutex(g_SendLock);

	if (--g_DeltaRecivedCound != 0) {
		release_mutex(g_SendLock);
		return;
	}

	g_ServerRecivedData = true;

	release_mutex(g_SendLock);
}

