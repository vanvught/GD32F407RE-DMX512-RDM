@echo off
setlocal EnableExtensions

rem Directory containing this .cmd file
set "DIR=%~dp0"

rem Prefer the Windows Python launcher, then fall back to python.exe
set "PYTHON_CMD="
where py >nul 2>&1
if not errorlevel 1 (
    set "PYTHON_CMD=py -3"
) else (
    where python >nul 2>&1
    if not errorlevel 1 set "PYTHON_CMD=python"
)

if not defined PYTHON_CMD (
    echo ERROR: No Python 3 installation found. 1>&2
    exit /b 1
)

set "VENV=%USERPROFILE%\venvs\gd32-common"
set "PY=%VENV%\Scripts\python.exe"

if not exist "%PY%" (
    echo Creating virtual environment: %VENV%
    %PYTHON_CMD% -m venv "%VENV%"
    if errorlevel 1 (
        echo ERROR: Failed to create virtual environment. 1>&2
        exit /b 1
    )
)

rem Install pyserial only when it is missing.
"%PY%" -c "import serial" >nul 2>&1
if errorlevel 1 (
    echo Installing pyserial...
    "%PY%" -m pip install -U pip
    if errorlevel 1 exit /b 1

    "%PY%" -m pip install -U pyserial
    if errorlevel 1 exit /b 1
)

rem Final dependency check.
"%PY%" -c "import serial; print('PySerial OK')"
if errorlevel 1 (
    echo ERROR: PySerial is not working. 1>&2
    exit /b 1
)

"%PY%" "%DIR%flash.py" %*
set "RC=%ERRORLEVEL%"

endlocal & exit /b %RC%
