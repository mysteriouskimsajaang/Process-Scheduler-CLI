CSOPESY MAJOR OUTPUT 1 — PROCESS SCHEDULER AND CLI
===================================================

Group 6 — Section: CSOPESY S03
Group members: Bowen Li, John Albert Teoxon, Kane Joshua Ibañez,
Matthew Drew Fabregas

Main entry point: src/main.cpp (int main)
Language: C++17

SOURCE FILES
------------
  src/main.cpp           CLI and command interpreter
  src/scheduler.hpp      Process, instruction, and scheduler declarations
  src/scheduler.cpp      Scheduling and instruction execution
  config.txt             Configuration loaded by initialize
  config-demo.txt        Short configuration for demonstrations
  config-fcfs.txt        FCFS demonstration configuration
  CMakeLists.txt         Build configuration
  tests/                 Automated scheduler and CLI tests
  TEST_PLAN.md           Manual demonstration checklist
  Technical_Report.pptx  Technical report presentation

BUILDING AND RUNNING ON WINDOWS
-------------------------------
Use the inner folder containing this README, src, and config.txt:

  D:\...\CSOPESY_Process_Scheduler

Open a Visual Studio 2022 Developer PowerShell and run:

  cd "path\to\CSOPESY_Process_Scheduler"
  cl /nologo /std:c++17 /EHsc /O2 src\main.cpp src\scheduler.cpp /Fe:csopesy.exe
  .\csopesy.exe

The program must be run from this folder because initialize loads config.txt
from the current working directory.

If csopesy.exe has already been built, run only:

  cd "path\to\CSOPESY_Process_Scheduler"
  .\csopesy.exe

The included build.bat is an optional MinGW-w64 build script. It requires a
modern g++ installation and is not needed when using Visual Studio MSVC.

BUILDING WITH CMAKE
-------------------
If the project path is long, use a short build directory:

  $source = (Get-Location).Path
  $build = "$env:USERPROFILE\csopesy-build"
  cmake -S $source -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build $build --target csopesy
  Set-Location $source
  & "$build\csopesy.exe"

CONFIGURATION
-------------
initialize reads config.txt. The file uses space-separated key/value pairs:

  num-cpu 4
  scheduler "rr"
  quantum-cycles 5
  batch-process-freq 1
  min-ins 1000
  max-ins 2000
  delays-per-exec 0

The scheduler can be:

  scheduler "rr"       Round Robin
  scheduler "fcfs"     First-Come, First-Served

The current config.txt is a short FCFS demonstration configuration. Change
only config.txt to test other instructor-provided parameters; recompilation is
not required.

COMMANDS
--------
Before initialize, only initialize and exit are accepted.

  initialize             Load config.txt and start the scheduler
  screen -s NAME         Create and attach to a new process
  screen -r NAME         Reattach to an active process
  screen -ls              List CPU utilization and processes
  scheduler-start        Start periodic dummy-process generation
  scheduler-stop         Stop generating new processes
  report-util            Write csopesy-log.txt
  exit                   Quit the emulator

Inside a process screen:

  process-smi            Display process progress and PRINT logs
  exit                   Return to the main menu

QUICK DEMONSTRATION
--------------------
For a short demonstration, copy config-demo.txt over config.txt before
launching. Then type:

  initialize
  screen -s demo
  process-smi
  exit
  scheduler-start
  screen -ls
  scheduler-stop
  report-util
  exit

The exact process status, CPU utilization, instruction count, and PRINT logs
vary because process instructions are generated randomly.

REPORT
------
The report-util command creates csopesy-log.txt. It contains CPU utilization
and summaries of active and finished processes.

TESTING
-------
Use TEST_PLAN.md as the manual checklist for the required demonstrations.
The assignment's MP4 evidence should show the configuration, commands, output,
and csopesy-log.txt when required by the instructor.

The optional automated tests can be built with a C++17 compiler:

  g++ -std=c++17 -O2 -Wall -Wextra -pthread ^
      tests\scheduler_tests.cpp src\scheduler.cpp -Isrc -o scheduler_tests.exe
  .\scheduler_tests.exe

SUBMISSION
----------
Prepare the following according to the assignment instructions:

  1. Source code and configuration files
  2. This README.txt with group and run information
  3. Technical_Report.pptx
  4. MP4 evidence for the assigned test cases

The Windows csopesy.exe file is optional; the source code is the primary
submission.
