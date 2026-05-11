@echo off
setlocal
set QTDIR=C:\Qt\6.11.0\mingw_64
set MINGW=C:\Qt\Tools\mingw1310_64
set PATH=%QTDIR%\bin;%MINGW%\bin;%PATH%

if not exist "dist\StegoLab.exe" (
    echo Сначала соберите проект: запустите ЗАПУСТИТЬ_СБОРКУ.bat
    pause
    exit /b 1
)

start "" /D "%~dp0dist" "%~dp0dist\StegoLab.exe"
