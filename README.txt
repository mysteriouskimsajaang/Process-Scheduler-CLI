CSOPESY MAJOR OUTPUT 1 — PROCESS SCHEDULER AND CLI
===================================================

Group 6 — Section: CSOPESY S03
Group members: Bowen Li, John Albert Teoxon, Kane Joshua Ibañez,
Matthew Drew Fabregas

Main entry point: src/main.cpp (int main)
Language: C++

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
  Technical_Report.pptx  Technical report presentation

BUILDING AND RUNNING ON WINDOWS
-------------------------------
Use the inner folder containing this README, src, and config.txt:

  D:\...\CSOPESY_Process_Scheduler

Option A (recommended): Visual Studio / MSVC
  1. Open "x64 Native Tools Command Prompt for VS 2022" (or the Developer
     PowerShell) from the Start menu. A plain PowerShell does not have cl.
  2. Run:

       cd "path\to\CSOPESY_Process_Scheduler"
       build-msvc.bat

     or, equivalently:

       cl /nologo /std:c++17 /EHsc /O2 src\main.cpp src\scheduler.cpp /Fe:csopesy.exe

Option B: MinGW-w64 g++ (build.bat)
  Requires g++ 9 or newer with POSIX threads. Old MinGW builds (for example
  the one bundled with Dev-C++ or Code::Blocks, or any g++ whose "g++ --version"
  is older than 9) fail with errors such as "'mutex' in namespace 'std' does
  not name a type" and "'clamp' is not a member of 'std'". To install a
  suitable compiler:
    1. Install MSYS2 from https://www.msys2.org
    2. In the "MSYS2 UCRT64" terminal run:
         pacman -S mingw-w64-ucrt-x86_64-gcc
    3. Add C:\msys64\ucrt64\bin to the FRONT of your Windows PATH.
    4. Open a new terminal, check "g++ --version", then run:
         cd "path\to\CSOPESY_Process_Scheduler"
         .\build.bat

Run the program from this folder, because initialize loads config.txt from the
current working directory:

  cd "path\to\CSOPESY_Process_Scheduler"
  .\csopesy.exe

If csopesy.exe was already built from the current source, skip the build step.
After changing any file in src\, rebuild; an old csopesy.exe does not pick up
source changes.

BUILDING ON LINUX / MACOS
-------------------------
  ./build.sh
  ./csopesy

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

The delay setting may be written as delays-per-exec or delay-per-exec (both are
accepted, but use only one of them in a file).

batch-process-freq N means scheduler-start / scheduler-test creates one new dummy
process every N * 200 ms (one simulated CPU tick is 20 ms, so N = 10 ticks).

The scheduler can be:

  scheduler "rr"       Round Robin
  scheduler "fcfs"     First-Come, First-Served

The current config.txt is the round-robin assignment demonstration configuration. Change
only config.txt to test other instructor-provided parameters; recompilation is
not required.

COMMANDS
--------
Before initialize, only initialize and exit are accepted.

  initialize             Load config.txt and start the scheduler
  screen -s NAME         Create and attach to a new process
  screen -r NAME         Reattach to an active process
  screen -ls             List CPU utilization, running processes (those on a
                         core) and finished processes
  scheduler-start        Start periodic dummy-process generation
                         (scheduler-test is accepted as an alias)
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
For the required demonstrations, the assignment's MP4 evidence should show the configuration, commands, output,
and csopesy-log.txt when required by the instructor.

The optional automated tests can be built with a C++17 compiler:

  g++ -std=c++17 -O2 -Wall -Wextra -pthread ^
      tests\scheduler_tests.cpp src\scheduler.cpp -Isrc -o scheduler_tests.exe
  .\scheduler_tests.exe

(or, with MSVC:
  cl /nologo /std:c++17 /EHsc /O2 /Isrc tests\scheduler_tests.cpp src\scheduler.cpp /Fe:scheduler_tests.exe)

Run the tests from this folder: they load config-demo.txt from the current
directory.

