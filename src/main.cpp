#include "scheduler.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#endif

using namespace std;
using namespace csopesy;

namespace {
//colors according to the ANSI table.
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string CYAN = "\033[36m";
const string OG = "\033[0m";

void showHelp();

void clearConsole() {
    std::cout << "\033[2J\033[H";
}

void enableAnsiWindows() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void runExitAnimation() {
    std::cout << "\n" << YELLOW << "Command Line Interface shutting down......" << OG;
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::cout << YELLOW << "." << std::flush;
    }
    std::cout << "\n\n";

    const int barWidth = 30;
    for (int i = 0; i <= barWidth; ++i) {
        std::cout << "\r" << CYAN << "[";
        for (int j = 0; j < i; ++j) std::cout << "=";
        if (i < barWidth) std::cout << ">";
        for (int j = (i < barWidth ? i + 1 : i); j < barWidth; ++j) std::cout << " ";
        std::cout << "] " << (i * 100 / barWidth) << "%" << OG << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    std::cout << "\n\n" << GREEN << "System shutdown complete. Goodbye!" << OG << "\n\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
}

void printHeader() {
    std::cout << R"(   ______ _____ ____  ____  ________________  )" << "\n";
    std::cout << R"(  / ____// ___// __ \/ __ \/ ____/ ___/\ \/ / )" << "\n";
    std::cout << R"( / /     \__ \/ / / / /_/ / __/  \__ \  \  /  )" << "\n";
    std::cout << R"(/ /___  ___/ / /_/ / ____/ /___ ___/ /  / /   )" << "\n";
    std::cout << R"(\____/ /____/\____/_/   /_____//____/  /_/    )" << "\n";
    std::cout << "\n";

    std::cout << GREEN << "Hello, Welcome to our CPU Scheduler and CLI Emulator!" << OG << "\n";
    std::cout << YELLOW << "Type 'exit' to quit, 'clear' to clear the screen" << OG << "\n\n";
    std::cout << "** IMPORTANT: Type 'initialize' to load config and start system **\n\n";
    showHelp();
}


void showHelp() {
    std::cout << "Commands:\n"
              << "  initialize             Load scheduler configuration from config.txt\n"
              << "  screen -s <name>       Create and attach to a process\n"
              << "  screen -r <name>       Reattach to a live process\n"
              << "  screen -ls             Show CPU utilization and all processes\n"
              << "  scheduler-start        Begin periodic dummy-process generation\n"
              << "  scheduler-stop         Stop generating new dummy processes\n"
              << "  report-util            Write csopesy-log.txt (includes finished processes)\n"
              << "  exit                   Quit the emulator\n";
}

void attachScreen(Scheduler& scheduler, const std::string& name) {
    clearConsole();
    std::cout << "Attached to '" << name << "'. Type process-smi to refresh, exit to detach.\n\n";
    ProcessSnapshot initial;
    if (scheduler.getProcess(name, initial)) std::cout << processReport(initial);
    std::string command;
    for (;;) {
        std::cout << "screen:" << name << "> ";
        std::cout.flush();
        if (!std::getline(std::cin, command)) break;
        if (command == "exit") {
            clearConsole();
            printHeader();
            return;
        }
        if (command == "process-smi") {
            ProcessSnapshot info;
            if (scheduler.getProcess(name, info)) std::cout << processReport(info);
            else std::cout << "Process not found.\n";
        } else if (!command.empty()) {
            std::cout << "Unknown screen command. Use process-smi or exit.\n";
        }
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc > 1) {
        std::cerr << "Usage: " << argv[0] << "\nRun from the folder containing config.txt.\n";
        return 1;
    }
    enableAnsiWindows();
    clearConsole();
    printHeader();
    std::unique_ptr<Scheduler> scheduler;
    std::string line;
    for (;;) {
        std::cout << "root:\\> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) break;
        std::istringstream stream(line);
        std::string command, option, name, leftover;
        stream >> command;
        if (command.empty()) continue;
        if (command == "exit" && !(stream >> leftover)) break;
        if (command == "clear" && !(stream >> leftover)) {
            clearConsole();
            printHeader();
            continue;
        }
        if (command == "initialize") {
            if (stream >> leftover) { std::cout << "Usage: initialize\n"; continue; }
            if (scheduler) { std::cout << "Scheduler already initialized.\n"; continue; }
            Config config;
            std::string error;
            if (!Config::load("config.txt", config, error)) {
                std::cout << "Initialization failed: " << error << '\n';
                continue;
            }
            scheduler = std::make_unique<Scheduler>(config);
            scheduler->start();
            std::cout << "Initialized " << config.numCpu << " CPU(s), "
                      << (config.algorithm == "rr" ? "round robin" : "first-come first-served")
                      << " scheduler. (" << kTickMilliseconds << " ms per simulated CPU tick)\n";
            continue;
        }
        if (!scheduler) {
            std::cout << "Please enter 'initialize' first (or 'exit').\n";
            continue;
        }
        if (command == "screen") {
            if (!(stream >> option)) { std::cout << "Usage: screen -s <name> | screen -r <name> | screen -ls\n"; continue; }
            if (option == "-ls" && !(stream >> leftover)) {
                std::cout << systemReport(scheduler->snapshot());
            } else if ((option == "-s" || option == "-r") && (stream >> name) && !(stream >> leftover)) {
                if (option == "-s") {
                    std::string error;
                    if (!scheduler->createProcess(name, error)) { std::cout << error << '\n'; continue; }
                    attachScreen(*scheduler, name);
                } else {
                    ProcessSnapshot p;
                    if (!scheduler->getProcess(name, p) || p.status == Status::Finished)
                        std::cout << "Process " << name << " not found.\n";
                    else attachScreen(*scheduler, name);
                }
            } else {
                std::cout << "Usage: screen -s <name> | screen -r <name> | screen -ls\n";
            }
        } else if (command == "scheduler-start" && !(stream >> leftover)) {
            scheduler->startGenerator();
            std::cout << "Scheduler process generation started.\n";
        } else if (command == "scheduler-stop" && !(stream >> leftover)) {
            scheduler->stopGenerator();
            std::cout << "Scheduler process generation stopped; existing processes continue.\n";
        } else if (command == "report-util" && !(stream >> leftover)) {
            const auto report = systemReport(scheduler->snapshot());
            std::ofstream out("csopesy-log.txt", std::ios::trunc);
            if (!out) std::cout << "Cannot write csopesy-log.txt in the current folder.\n";
            else {
                out << report;
                if (out.good()) std::cout << "Report generated at csopesy-log.txt\n";
                else std::cout << "Failed to write report.\n";
            }
        } else {
            std::cout << "Unrecognized command. Valid commands: screen, scheduler-start, "
                         "scheduler-stop, report-util, exit.\n";
        }
    }
    if (scheduler) scheduler->stop();
    runExitAnimation();
    return 0;
}
