@echo off
setlocal EnableDelayedExpansion

set OUTPUT_DIR=%~dp0build

echo INFO: Checking CMake in path
where cmake >nul 2>nul || (
  echo ERROR: CMake not found in path
  exit /B !ERRORLEVEL!
)

echo INFO: Checking if output directory exist
if not exist "%OUTPUT_DIR%" md "%OUTPUT_DIR%" || (
  >&2 echo ERROR: Create output dir "%OUTPUT_DIR%" failed with exit code !ERRORLEVEL!
  exit /B !ERRORLEVEL!
)

call :RUN_BUILD
if %ERRORLEVEL% neq 0 (
  >&2 echo ERROR: Build failed with exit code %ERRORLEVEL%
  exit /B %ERRORLEVEL%
)

exit /B 0

:RUN_BUILD
setlocal
pushd %OUTPUT_DIR% || (
  >&2 echo ERROR: Cannot find output directory
  exit /B !ERRORLEVEL!
)

rem Prepare project
echo INFO: Checking CMake cache
if not exist "%CD%\CMakeCache.txt" (
  echo INFO: Generating CMake cache
  cmake .. || (
    >&2 echo ERROR: CMake failed to generate project files
	popd
	exit /B !ERRORLEVEL!
  )
)

rem Run build with CMake so we will be sure we are calling correct builder
echo INFO: Runnign build
cmake --build .
popd
if %ERRORLEVEL% neq 0 (
  >&2 echo ERROR: CMake failed with exit code %ERRORLEVEL%
  exit /B %ERRORLEVEL%
)

exit /B 0
