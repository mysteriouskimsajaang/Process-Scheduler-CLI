@echo off
cd /d "%~dp0"
g++ -std=c++17 -O2 -Wall -Wextra -pthread src\main.cpp src\scheduler.cpp -o csopesy.exe
if errorlevel 1 exit /b 1
echo Built csopesy.exe
