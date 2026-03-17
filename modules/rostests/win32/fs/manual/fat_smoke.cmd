@echo off
setlocal EnableExtensions EnableDelayedExpansion

if "%~1"=="" (
    echo Usage: fat_smoke.cmd DRIVE:
    exit /b 2
)

set "TARGET=%~d1\"
if not exist "%TARGET%." (
    echo Target drive not found: %TARGET%
    exit /b 2
)

set "LETTER=%TARGET:~0,1%"
set "RUNID=%RANDOM%%RANDOM%"
set "BASE=%TARGET%fatvm_%RUNID%"
if not defined TEMP set "TEMP=%TARGET%"
set "LOG=%TEMP%\fatvm_smoke_%LETTER%_%RUNID%.log"

echo FAT smoke test started on %TARGET%
echo FAT smoke test started on %TARGET%>"%LOG%"
echo Log file: %LOG%
>>"%LOG%" echo Log file: %LOG%

mkdir "%BASE%" >nul 2>&1
if errorlevel 1 goto :mkdir_fail

mkdir "%BASE%\tree\child" >nul 2>&1
if errorlevel 1 goto :tree_fail

> "%BASE%\alpha.txt" echo alpha
if errorlevel 1 goto :alpha_fail

> "%BASE%\bulk.txt" echo bulk-begin
if errorlevel 1 goto :bulk_create_fail

for /L %%N in (1,1,2048) do @echo line %%N>> "%BASE%\bulk.txt"
if errorlevel 1 goto :bulk_extend_fail

copy /y "%BASE%\bulk.txt" "%BASE%\bulk_copy.txt" >nul
if errorlevel 1 goto :copy_fail
echo PASS: Copy succeeded
>>"%LOG%" echo PASS: Copy succeeded

ren "%BASE%\bulk_copy.txt" "bulk_renamed.txt"
if errorlevel 1 goto :rename_fail
echo PASS: Rename succeeded
>>"%LOG%" echo PASS: Rename succeeded

move /y "%BASE%\bulk_renamed.txt" "%BASE%\tree\child\bulk_moved.txt" >nul
if errorlevel 1 goto :move_fail
echo PASS: Move succeeded
>>"%LOG%" echo PASS: Move succeeded

copy /y "%BASE%\tree\child\bulk_moved.txt" "%BASE%\bulk_second.txt" >nul
if errorlevel 1 goto :second_copy_fail

type "%BASE%\tree\child\bulk_moved.txt" >> "%BASE%\bulk_second.txt"
if errorlevel 1 goto :append_fail
echo PASS: Growth append succeeded
>>"%LOG%" echo PASS: Growth append succeeded

move /y "%BASE%\alpha.txt" "%BASE%\tree\alpha_moved.txt" >nul
if errorlevel 1 goto :alpha_move_fail

del /f /q "%BASE%\tree\alpha_moved.txt" >nul 2>&1
if errorlevel 1 goto :delete_fail
echo PASS: Delete succeeded
>>"%LOG%" echo PASS: Delete succeeded

ren "%BASE%\tree" "tree_renamed"
if errorlevel 1 goto :dir_rename_fail
echo PASS: Directory rename succeeded
>>"%LOG%" echo PASS: Directory rename succeeded

dir /s "%BASE%\tree_renamed" > "%BASE%\dir_snapshot.txt"
if errorlevel 1 goto :dir_enum_fail
echo PASS: Directory enumeration succeeded
>>"%LOG%" echo PASS: Directory enumeration succeeded

if not exist "%BASE%\bulk.txt" goto :missing_bulk
if not exist "%BASE%\tree_renamed\child\bulk_moved.txt" goto :missing_moved
if not exist "%BASE%\bulk_second.txt" goto :missing_grown
if exist "%BASE%\tree_renamed\alpha_moved.txt" goto :delete_stale

for %%I in ("%BASE%\bulk.txt") do set "SIZE1=%%~zI"
for %%I in ("%BASE%\bulk_second.txt") do set "SIZE2=%%~zI"

if not defined SIZE1 goto :size1_fail
if not defined SIZE2 goto :size2_fail
if !SIZE2! LEQ !SIZE1! goto :growth_size_fail

echo PASS: File growth size check succeeded
>>"%LOG%" echo PASS: File growth size check succeeded
echo RESULT: PASS
>>"%LOG%" echo RESULT: PASS
exit /b 0

:mkdir_fail
echo FAIL: Could not create base directory %BASE%
>>"%LOG%" echo FAIL: Could not create base directory %BASE%
exit /b 1

:tree_fail
echo FAIL: Could not create directory tree
>>"%LOG%" echo FAIL: Could not create directory tree
exit /b 1

:alpha_fail
echo FAIL: Could not create alpha.txt
>>"%LOG%" echo FAIL: Could not create alpha.txt
exit /b 1

:bulk_create_fail
echo FAIL: Could not create bulk.txt
>>"%LOG%" echo FAIL: Could not create bulk.txt
exit /b 1

:bulk_extend_fail
echo FAIL: Could not extend bulk.txt
>>"%LOG%" echo FAIL: Could not extend bulk.txt
exit /b 1

:copy_fail
echo FAIL: Copy bulk.txt -> bulk_copy.txt failed
>>"%LOG%" echo FAIL: Copy bulk.txt -> bulk_copy.txt failed
exit /b 1

:rename_fail
echo FAIL: Rename bulk_copy.txt -> bulk_renamed.txt failed
>>"%LOG%" echo FAIL: Rename bulk_copy.txt -> bulk_renamed.txt failed
exit /b 1

:move_fail
echo FAIL: Move bulk_renamed.txt -> tree\child\bulk_moved.txt failed
>>"%LOG%" echo FAIL: Move bulk_renamed.txt -> tree\child\bulk_moved.txt failed
exit /b 1

:second_copy_fail
echo FAIL: Copy bulk_moved.txt -> bulk_second.txt failed
>>"%LOG%" echo FAIL: Copy bulk_moved.txt -> bulk_second.txt failed
exit /b 1

:append_fail
echo FAIL: Append into bulk_second.txt failed
>>"%LOG%" echo FAIL: Append into bulk_second.txt failed
exit /b 1

:alpha_move_fail
echo FAIL: Move alpha.txt -> tree\alpha_moved.txt failed
>>"%LOG%" echo FAIL: Move alpha.txt -> tree\alpha_moved.txt failed
exit /b 1

:delete_fail
echo FAIL: Delete tree\alpha_moved.txt failed
>>"%LOG%" echo FAIL: Delete tree\alpha_moved.txt failed
exit /b 1

:dir_rename_fail
echo FAIL: Rename tree -> tree_renamed failed
>>"%LOG%" echo FAIL: Rename tree -> tree_renamed failed
exit /b 1

:dir_enum_fail
echo FAIL: Directory enumeration failed
>>"%LOG%" echo FAIL: Directory enumeration failed
exit /b 1

:missing_bulk
echo FAIL: Missing bulk.txt after operations
>>"%LOG%" echo FAIL: Missing bulk.txt after operations
exit /b 1

:missing_moved
echo FAIL: Missing moved file after operations
>>"%LOG%" echo FAIL: Missing moved file after operations
exit /b 1

:missing_grown
echo FAIL: Missing grown file after operations
>>"%LOG%" echo FAIL: Missing grown file after operations
exit /b 1

:delete_stale
echo FAIL: Deleted file still exists
>>"%LOG%" echo FAIL: Deleted file still exists
exit /b 1

:size1_fail
echo FAIL: Could not read size of bulk.txt
>>"%LOG%" echo FAIL: Could not read size of bulk.txt
exit /b 1

:size2_fail
echo FAIL: Could not read size of bulk_second.txt
>>"%LOG%" echo FAIL: Could not read size of bulk_second.txt
exit /b 1

:growth_size_fail
echo FAIL: bulk_second.txt did not grow as expected
>>"%LOG%" echo FAIL: bulk_second.txt did not grow as expected
exit /b 1
