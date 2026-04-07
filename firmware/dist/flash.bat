@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "ENV_NAME=%~1"
if "%ENV_NAME%"=="" set "ENV_NAME=release"

set "LOCALE=%~2"
if "%LOCALE%"=="" set "LOCALE=en"

if /I "%LOCALE%"=="en" (
    set "LOCALE=en"
) else if /I "%LOCALE%"=="de" (
    set "LOCALE=de"
) else (
    >&2 echo Unsupported locale "%LOCALE%". Use "en" or "de".
    exit /b 1
)

set "ARTIFACT_BASE=firmware_%LOCALE%"
set "HEX_PATH=%SCRIPT_DIR%\%ENV_NAME%\%ARTIFACT_BASE%.hex"

if not defined AVRDUDE_BIN set "AVRDUDE_BIN=avrdude"
if not defined MCU set "MCU=attiny88"
if not defined PROGRAMMER set "PROGRAMMER=atmelice_isp"
if not defined PORT set "PORT=usb"

if not exist "%HEX_PATH%" (
    >&2 echo Missing firmware artifact "%HEX_PATH%". Build or copy the desired .hex into firmware\dist\%ENV_NAME%\ first.
    exit /b 1
)

"%AVRDUDE_BIN%" ^
    -p "%MCU%" ^
    -c "%PROGRAMMER%" ^
    -P "%PORT%" ^
    -V ^
    -U lfuse:w:0xee:m ^
    -U hfuse:w:0xdf:m ^
    -U efuse:w:0xff:m ^
    -U flash:w:"%HEX_PATH%":i

exit /b %ERRORLEVEL%
