taskkill /F /IM URemoteControlService.exe
sc delete URemoteControlService

set CONFIG_FILE_NAME="config.ini"
del %CONFIG_FILE_NAME%