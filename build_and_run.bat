@echo off
setlocal

g++ -std=c++11 -Wall -Wextra -pedantic main.cpp -o parser.exe
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo === Sample A: valid input ===
parser.exe SampleA_Valid.txt
if errorlevel 1 exit /b 1

echo.
echo === Sample B: invalid input ===
parser.exe SampleB_Invalid.txt
if errorlevel 1 exit /b 1

echo.
echo Generated outputs are in the project folder.
echo Reference outputs are in outputs\valid and outputs\invalid.
endlocal
