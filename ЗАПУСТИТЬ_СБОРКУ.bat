@echo off
setlocal
title StegoLab Build - Qt 6.11.0 MinGW
color 0A

echo ============================================
echo  StegoLab - Сборка (Qt 6.11.0 + MinGW)
echo ============================================
echo.

set MINGW=C:\Qt\Tools\mingw1310_64
set QTDIR=C:\Qt\6.11.0\mingw_64
set CMAKE=C:\Qt\Tools\CMake_64\bin

set PATH=%QTDIR%\bin;%MINGW%\bin;%CMAKE%;%PATH%

echo [+] MinGW : %MINGW%
echo [+] Qt    : %QTDIR%
echo.

if not exist "%MINGW%\bin\gcc.exe"    ( echo [X] GCC не найден! & pause & exit /b 1 )
if not exist "%QTDIR%\bin\qmake6.exe" ( echo [X] Qt не найден!  & pause & exit /b 1 )
if not exist "%CMAKE%\cmake.exe"      ( echo [X] CMake не найден! & pause & exit /b 1 )

echo [OK] Все компоненты найдены.
echo.

if not exist build_mingw mkdir build_mingw
cd build_mingw

echo [1/3] CMake конфигурация...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QTDIR%" -DCMAKE_C_COMPILER="%MINGW%\bin\gcc.exe" -DCMAKE_CXX_COMPILER="%MINGW%\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="%MINGW%\bin\mingw32-make.exe"

if %errorlevel% neq 0 ( echo [X] CMake ошибка! & cd .. & pause & exit /b 1 )

echo.
echo [2/3] Компиляция...
mingw32-make -j4

if %errorlevel% neq 0 ( echo [X] Ошибка компиляции! & cd .. & pause & exit /b 1 )
cd ..

echo.
echo [3/3] Создание dist\ для флешки...
if not exist dist mkdir dist
copy build_mingw\StegoLab.exe dist\ >nul

"%QTDIR%\bin\windeployqt6.exe" --release --no-translations --dir dist dist\StegoLab.exe

for %%D in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    if exist "%MINGW%\bin\%%D" if not exist "dist\%%D" copy "%MINGW%\bin\%%D" dist\ >nul
)

echo.
echo ============================================
echo  ГОТОВО!
echo  Запустить:  dist\StegoLab.exe
echo  На флешку:  скопируйте папку dist\ целиком
echo ============================================
echo.
start "" dist\StegoLab.exe
pause
