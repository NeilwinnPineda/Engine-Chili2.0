@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "BUILD_DIR=%REPO_ROOT%\build"
set "LOG_DIR=%REPO_ROOT%\logs"
set "TEST_LOG=%LOG_DIR%\ctest.log"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    call "%REPO_ROOT%\configure.cmd"
    if errorlevel 1 exit /b %ERRORLEVEL%
)

set "CTEST_EXE=ctest"
where ctest >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files\CMake\bin\ctest.exe" (
        set "CTEST_EXE=C:\Program Files\CMake\bin\ctest.exe"
    ) else (
        echo CTest was not found on PATH or at C:\Program Files\CMake\bin\ctest.exe.
        exit /b 1
    )
)

echo ==== TEST START %DATE% %TIME% ==== > "%TEST_LOG%"
"%CTEST_EXE%" --test-dir "%BUILD_DIR%" --output-on-failure >> "%TEST_LOG%" 2>&1
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    echo Tests failed with exit code %RESULT%.
    echo See "%TEST_LOG%".
    exit /b %RESULT%
)

echo Tests complete.
echo Log: "%TEST_LOG%"
exit /b 0
