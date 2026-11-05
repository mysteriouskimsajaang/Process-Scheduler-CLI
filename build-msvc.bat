@echo off
rem Build with Visual Studio's compiler (cl). Run from "x64 Native Tools Command Prompt for VS".
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo cl.exe not found. Open "x64 Native Tools Command Prompt for VS 2022" from the Start menu, then run build-msvc.bat again.
  exit /b 1
)
if not exist obj mkdir obj
cl /nologo /std:c++17 /EHsc /O2 /W3 /Fo:obj\ src\main.cpp src\scheduler.cpp /Fe:csopesy.exe
if errorlevel 1 exit /b 1
echo Built csopesy.exe
