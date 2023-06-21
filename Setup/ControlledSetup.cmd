set BIN_PATH=%~dp0
set SERVER_EXE_NAME=URemoteControlService.exe

set FULL_SERVICE_PATH="%BIN_PATH%%SERVER_EXE_NAME%"

sc create URemoteControlService binPath=%FULL_SERVICE_PATH%
sc failure URemoteControlService reset= 30 actions=restart
sc config URemoteControlService start=auto


set CONFIG_FILE_NAME="config.ini"

type nul >%CONFIG_FILE_NAME%

(
echo [config]
echo target_width=1920
echo target_height=1080
echo target_bit_count=16
echo capture_full_screen_interval=5000
echo // max value is 9
echo compression_level=6
echo max_send_buffer=16000
echo // max value is 256
echo num_of_delta_parts=50
echo [ServerConnectInfo]
echo http_server_url="http://<ip_address>:8080/"
echo main_server_address="<ip_address>"
) >%CONFIG_FILE_NAME% 