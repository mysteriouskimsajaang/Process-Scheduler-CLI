#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace csopesy {

constexpr int kTickMilliseconds = 20;

struct Config {
    int numCpu = 4;
    std::string algorithm = "rr";
    int quantumCycles = 5;
    int batchFrequency = 1;
    int minInstructions = 1000;
    int maxInstructions = 2000;
    int delaysPerExec = 0;

    static bool load(const std::string& filename, Config& out, std::string& error);
};

enum class Status { Ready, Running, Sleeping, Finished };
enum class Op { Print, Declare, Add, Subtract, Sleep, For };

struct Instruction {
    Op op = Op::Print;
    std::string a, b, c;
    int ticks = 0;
    int repeats = 0;
    bool printVariable = false;
    std::vector<Instruction> body;
};

// A single running process is a virtual, scheduled CPU task. Its program may
// contain nested FOR instructions; the executed instruction stream is expanded
// once when the process is created, so the program counter is unambiguous.
struct Process {
    std::string name;
    unsigned long long pid = 0;
    Status status = Status::Ready;
    std::vector<Instruction> executedInstructions;
    std::size_t pc = 0;
    int lastCore = -1;
    int sliceUsed = 0;
    int cooldown = 0;
    std::uint64_t wakeTick = 0;
    std::unordered_map<std::string, std::uint16_t> variables;
    std::vector<std::string> logs;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point finishedAt;
};

struct ProcessSnapshot {
    std::string name;
    unsigned long long pid = 0;
    Status status = Status::Ready;
    std::size_t currentInstruction = 0;
    std::size_t totalInstructions = 0;
    int lastCore = -1;
    std::uint64_t wakeTick = 0;
    std::vector<std::string> logs;
    std::unordered_map<std::string, std::uint16_t> variables;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point finishedAt;
};

struct SystemSnapshot {
    std::uint64_t tick = 0;
    int cpuCount = 0;
    int coresUsed = 0;
    bool generatorActive = false;
    std::vector<ProcessSnapshot> processes;
};

class Scheduler {
public:
    explicit Scheduler(Config config);
    ~Scheduler();
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void start();
    void stop();
    bool createProcess(const std::string& name, std::string& error);
    bool getProcess(const std::string& name, ProcessSnapshot& result) const;
    SystemSnapshot snapshot() const;
    void startGenerator();
    void stopGenerator();
    // For automated testing without wall-clock delays; do not call while start() is active.
    void advanceForTest(int ticks);
    // Test-only creation of a deterministic instruction program.
    void createProgramForTest(const std::string& name, const std::vector<Instruction>& program);

    const Config& config() const { return config_; }

private:
    using ProcessPtr = std::shared_ptr<Process>;
    void tickLocked();
    void createLocked(const std::string& name);
    std::vector<Instruction> generateBlock(int budget, int depth);
    Instruction randomLeaf();
    static void flatten(const std::vector<Instruction>& input, std::vector<Instruction>& output);
    static std::uint16_t clamp16(std::int64_t x);
    static std::uint16_t valueOf(Process& process, const std::string& argument);
    void executeInstruction(Process& process);
    void assignCore(int core);
    void freeCore(int core);
    ProcessSnapshot copyProcess(const Process& p) const;
    void backgroundLoop();

    Config config_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mt19937 random_;
    std::uint64_t tick_ = 0;
    unsigned long long nextPid_ = 1;
    unsigned long long nextGeneratedName_ = 1;
    bool generating_ = false;
    std::uint64_t nextGenerationTick_ = 0;
    std::unordered_map<std::string, ProcessPtr> byName_;
    std::vector<ProcessPtr> createdOrder_;
    std::deque<ProcessPtr> ready_;
    std::vector<ProcessPtr> cores_;
};

std::string formatTime(std::chrono::system_clock::time_point when);
std::string statusText(Status status);
std::string systemReport(const SystemSnapshot& snapshot);
std::string processReport(const ProcessSnapshot& process);

} // namespace csopesy
