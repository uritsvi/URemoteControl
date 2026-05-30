@echo off
REM Generate a self-signed certificate for TLS (development/testing only)
REM Usage: run from Server directory or specify output path

set CERT_DIR=%~dp0certs
if not exist "%CERT_DIR%" mkdir "%CERT_DIR%"

echo Generating self-signed certificate...
openssl req -x509 -newkey rsa:2048 -keyout "%CERT_DIR%\server.key" -out "%CERT_DIR%\server.crt" -days 365 -nodes -subj "/CN=localhost"

if %ERRORLEVEL% neq 0 (
    echo Failed to generate certificate. Ensure OpenSSL is installed and in PATH.
    exit /b 1
)

echo.
echo Certificate generated successfully.
echo   Certificate: %CERT_DIR%\server.crt
echo   Private key: %CERT_DIR%\server.key
echo.
echo To enable TLS on the server, set environment variables before starting:
echo   set UREMOTE_TLS_CERT=%CERT_DIR%\server.crt
echo   set UREMOTE_TLS_KEY=%CERT_DIR%\server.key
echo.
echo For client config.ini, add:
echo   use_tls=1
echo   tls_server_name=localhost
echo   tls_ca_cert_path=path\to\server.crt
echo.
