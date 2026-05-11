@echo off
setlocal
title Сборка курсовой работы
color 0A

echo ============================================
echo  Сборка проекта
echo ============================================
echo.

set MINGW=C:\Qt\Tools\mingw1310_64
set QTDIR=C:\Qt\6.11.0\mingw_64
set CMAKE=C:\Qt\Tools\CMake_64\bin

set PATH=%QTDIR%\bin;%MINGW%\bin;%CMAKE%;%PATH%

if not exist "%MINGW%\bin\gcc.exe"    ( echo [X] MinGW не найден! & pause & exit /b 1 )
if not exist "%QTDIR%\bin\Qt6Core.dll" ( echo [X] Qt не найден!  & pause & exit /b 1 )
if not exist "%CMAKE%\cmake.exe"      ( echo [X] CMake не найден! & pause & exit /b 1 )

echo [OK] Компоненты найдены
echo.

if not exist build_mingw mkdir build_mingw
cd build_mingw

echo [1/3] CMake конфигурация...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QTDIR%"
if %errorlevel% neq 0 ( echo [X] CMake ошибка! & cd .. & pause & exit /b 1 )

echo.
echo [2/3] Компиляция...
mingw32-make -j4
if %errorlevel% neq 0 ( echo [X] Ошибка компиляции! & cd .. & pause & exit /b 1 )
cd ..

echo.
echo [3/3] Развёртывание Qt DLL...
if not exist dist mkdir dist
copy build_mingw\StegoLab.exe dist\ >nul

"%QTDIR%\bin\windeployqt6.exe" --release --no-translations --dir dist dist\StegoLab.exe >nul 2>&1
if %errorlevel% neq 0 (
    "%QTDIR%\bin\windeployqt.exe" --release --no-translations --dir dist dist\StegoLab.exe >nul 2>&1
)

for %%D in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    if exist "%MINGW%\bin\%%D" if not exist "dist\%%D" copy "%MINGW%\bin\%%D" dist\ >nul
)

echo.
echo ============================================
echo  ГОТОВО!
echo  Запуск: dist\StegoLab.exe
echo ============================================
echo.
start "" /D "%~dp0dist" "%~dp0dist\StegoLab.exe"
pause
