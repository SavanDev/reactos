@echo off
setlocal EnableExtensions EnableDelayedExpansion

if "%~1"=="" goto :usage

set "TARGET=%~d1\"
if not exist "%TARGET%." (
    echo Target drive not found: %TARGET%
    exit /b 2
)

set "LETTER=%TARGET:~0,1%"
set "RUNID=%RANDOM%%RANDOM%"
set "BASE=%TARGET%fatdirty_%RUNID%"
if not defined TEMP set "TEMP=%TARGET%"
set "LOG=%TEMP%\fatvm_dirty_%LETTER%_%RUNID%.log"
set /a CYCLE=0

mkdir "%BASE%\queue" >nul 2>&1
if errorlevel 1 (
    echo Could not create workload directory %BASE%
    exit /b 1
)

echo FAT dirty loop started on %TARGET%
echo Hard power off the VM while this script is still running.
echo Log file: %LOG%
>> "%LOG%" echo FAT dirty loop started on %TARGET%
>> "%LOG%" echo Hard power off the VM while this script is still running.

:loop
set /a CYCLE+=1

> "%BASE%\current.txt" echo cycle !CYCLE!
for /L %%N in (1,1,256) do @echo !CYCLE! %%N>> "%BASE%\current.txt"

copy /y "%BASE%\current.txt" "%BASE%\queue\cycle!CYCLE!.txt" >nul
type "%BASE%\current.txt" >> "%BASE%\journal.txt"
move /y "%BASE%\current.txt" "%BASE%\queue\current_!CYCLE!.txt" >nul

if !CYCLE! GTR 6 (
    set /a OLD=CYCLE-6
    del /f /q "%BASE%\queue\cycle!OLD!.txt" >nul 2>&1
    del /f /q "%BASE%\queue\current_!OLD!.txt" >nul 2>&1
)

if !CYCLE! EQU 1 (
    >> "%LOG%" echo Workload loop is active.
)

if !CYCLE! GEQ 1000000 set /a CYCLE=100

ping -n 2 127.0.0.1 >nul
goto loop

:usage
echo Usage: fat_dirty_loop.cmd DRIVE:
exit /b 2
