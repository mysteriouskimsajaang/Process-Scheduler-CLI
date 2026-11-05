#include "scheduler.hpp"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace csopesy {
namespace {
std::string trim(std::string value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

int uniform(std::mt19937& gen, int minVal, int maxVal) {
    return std::uniform_int_distribution<int>(minVal, maxVal)(gen);
}

bool numeric(const std::string& value) {
    if (value.empty()) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); });
}
} // namespace

bool Config::load(const std::string& filename, Config& out, std::string& error) {
    std::ifstream file(filename);
    if (!file) {
        error = "Cannot open " + filename + ". Put config.txt beside the executable's working directory.";
        return false;
    }
    std::unordered_map<std::string, std::string> settings;
    std::string line;
    int lineNo = 0;
    const std::unordered_set<std::string> validKeys = {
        "num-cpu", "scheduler", "quantum-cycles", "batch-process-freq",
        "min-ins", "max-ins", "delay-per-exec", "delays-per-exec"
    };
    while (std::getline(file, line)) {
        ++lineNo;
        auto pos = line.find('#');
        if (pos != std::string::npos) line.erase(pos);
        line = trim(line);
        if (line.empty()) continue;
        std::istringstream in(line);
        std::string key, value, extra;
        if (!(in >> key >> value) || (in >> extra)) {
            error = "Invalid config.txt line " + std::to_string(lineNo) + ": expected 'key value'.";
            return false;
        }
        if (!validKeys.count(key)) {
            error = "Unknown config setting '" + key + "' on line " + std::to_string(lineNo) + ".";
            return false;
        }
        if (!settings.emplace(key, value).second) {
            error = "Duplicate config setting '" + key + "'.";
            return false;
        }
    }
    for (const auto& key : validKeys) {
        // "delay-per-exec" and "delays-per-exec" are aliases: either one is enough.
        if (key == "delay-per-exec" || key == "delays-per-exec") continue;
        if (!settings.count(key)) {
            error = "Missing required setting '" + key + "'.";
            return false;
        }
    }
    if (!settings.count("delay-per-exec") && !settings.count("delays-per-exec")) {
        error = "Missing required setting 'delay-per-exec'.";
        return false;
    }
    if (settings.count("delay-per-exec") && settings.count("delays-per-exec")) {
        error = "Specify only one of 'delay-per-exec' or 'delays-per-exec'.";
        return false;
    }
    auto readInt = [&](const std::string& key, int low, int high, int& dest) -> bool {
        const auto& raw = settings.at(key);
        if (!numeric(raw)) {
            error = "'" + key + "' must be an integer in [" + std::to_string(low) + ", " + std::to_string(high) + "].";
            return false;
        }
        try {
            const long long number = std::stoll(raw);
            if (number < low || number > high) throw std::out_of_range("config");
            dest = static_cast<int>(number);
            return true;
        } catch (const std::exception&) {
            error = "'" + key + "' must be in [" + std::to_string(low) + ", " + std::to_string(high) + "].";
            return false;
        }
    };
    Config parsed;
    if (!readInt("num-cpu", 1, 128, parsed.numCpu) ||
        !readInt("quantum-cycles", 1, 1000000, parsed.quantumCycles) ||
        !readInt("batch-process-freq", 1, 1000000, parsed.batchFrequency) ||
        !readInt("min-ins", 1, 1000000, parsed.minInstructions) ||
        !readInt("max-ins", 1, 1000000, parsed.maxInstructions) ||
        !(settings.count("delay-per-exec")
              ? readInt("delay-per-exec", 0, 1000000, parsed.delaysPerExec)
              : readInt("delays-per-exec", 0, 1000000, parsed.delaysPerExec))) return false;
    if (parsed.minInstructions > parsed.maxInstructions) {
        error = "min-ins must be less than or equal to max-ins.";
        return false;
    }
    parsed.algorithm = settings.at("scheduler");
    if (parsed.algorithm.size() > 1 && parsed.algorithm.front() == '"' && parsed.algorithm.back() == '"') {
        parsed.algorithm = parsed.algorithm.substr(1, parsed.algorithm.size() - 2);
    }
    if (parsed.algorithm != "fcfs" && parsed.algorithm != "rr") {
        error = "scheduler must be 'fcfs' or 'rr' (quotes optional).";
        return false;
    }
    out = parsed;
    return true;
}

std::string formatTime(std::chrono::system_clock::time_point when) {
    const auto time = std::chrono::system_clock::to_time_t(when);
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    std::ostringstream out;
    out << std::put_time(&result, "%m/%d/%Y %I:%M:%S%p");
    return out.str();
}

std::string statusText(Status status) {
    switch (status) {
        case Status::Ready: return "READY";
        case Status::Running: return "RUNNING";
        case Status::Sleeping: return "SLEEPING";
        case Status::Finished: return "FINISHED";
    }
    return "UNKNOWN";
}

Scheduler::Scheduler(Config config)
    : config_(std::move(config)), random_(std::random_device{}()), cores_(static_cast<std::size_t>(config_.numCpu)) {}

Scheduler::~Scheduler() { stop(); }

void Scheduler::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&Scheduler::backgroundLoop, this);
}

void Scheduler::stop() {
    running_.store(false);
    if (worker_.joinable()) worker_.join();
}

void Scheduler::backgroundLoop() {
    auto next = std::chrono::steady_clock::now();
    while (running_.load()) {
        next += std::chrono::milliseconds(kTickMilliseconds);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tickLocked();
        }
        std::this_thread::sleep_until(next);
        // Don't perform a huge backlog after terminal/system suspension.
        if (std::chrono::steady_clock::now() > next + std::chrono::milliseconds(1000))
            next = std::chrono::steady_clock::now();
    }
}

void Scheduler::advanceForTest(int ticks) {
    if (running_.load()) throw std::logic_error("Cannot advance tests while worker is running.");
    std::lock_guard<std::mutex> lock(mutex_);
    for (int i = 0; i < ticks; ++i) tickLocked();
}

bool Scheduler::createProcess(const std::string& name, std::string& error) {
    if (name.empty() || name.size() > 64 || !std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-';
        })) {
        error = "Process name must be 1-64 characters: letters, digits, underscore or hyphen.";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (byName_.count(name)) {
        error = "Process '" + name + "' already exists (finished names are reserved).";
        return false;
    }
    createLocked(name);
    return true;
}

std::vector<Instruction> Scheduler::generateAlternating(int count) {
    std::vector<Instruction> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        Instruction inst;
        if (i % 2 == 0) {
            // PRINT("Value from: " + x)
            inst.op = Op::Print;
            inst.a = "Value from: ";
            inst.b = "x";
            inst.printVariable = true;
        } else {
            // ADD(x, x, [1 - 10])
            inst.op = Op::Add;
            inst.a = "x";
            inst.b = "x";
            inst.c = std::to_string(uniform(random_, 1, 10));
        }
        result.push_back(std::move(inst));
    }
    return result;
}

void Scheduler::flatten(const std::vector<Instruction>& input, std::vector<Instruction>& output) {
    for (const auto& inst : input) {
        if (inst.op != Op::For) {
            output.push_back(inst);
        } else {
            for (int i = 0; i < inst.repeats; ++i) flatten(inst.body, output);
        }
    }
}

void Scheduler::createLocked(const std::string& name) {
    auto process = std::make_shared<Process>();
    process->name = name;
    process->pid = nextPid_++;
    process->createdAt = std::chrono::system_clock::now();
    const int instructions = uniform(random_, config_.minInstructions, config_.maxInstructions);
    const auto program = generateAlternating(instructions);
    process->variables["x"] = 0; // every process starts with x = 0
    process->executedInstructions.reserve(static_cast<std::size_t>(instructions));
    flatten(program, process->executedInstructions);
    byName_.emplace(process->name, process);
    createdOrder_.push_back(process);
    ready_.push_back(process);
}

void Scheduler::createProgramForTest(const std::string& name, const std::vector<Instruction>& program) {
    if (running_.load()) throw std::logic_error("Cannot add test programs while worker is running.");
    std::lock_guard<std::mutex> lock(mutex_);
    if (byName_.count(name)) throw std::invalid_argument("Duplicate test process");
    auto p = std::make_shared<Process>();
    p->name = name;
    p->pid = nextPid_++;
    p->createdAt = std::chrono::system_clock::now();
    flatten(program, p->executedInstructions);
    byName_.emplace(p->name, p);
    createdOrder_.push_back(p);
    ready_.push_back(p);
}

std::uint16_t Scheduler::clamp16(std::int64_t x) {
    return static_cast<std::uint16_t>(std::clamp<std::int64_t>(x, 0, 65535));
}

std::uint16_t Scheduler::valueOf(Process& process, const std::string& argument) {
    if (numeric(argument)) return clamp16(std::stoll(argument));
    return process.variables[argument]; // automatically declares missing vars as zero
}

void Scheduler::executeInstruction(Process& process) {
    if (process.pc >= process.executedInstructions.size()) {
        process.status = Status::Finished;
        process.finishedAt = std::chrono::system_clock::now();
        return;
    }
    const Instruction& inst = process.executedInstructions[process.pc];
    ++process.pc;
    switch (inst.op) {
        case Op::Print: {
            std::string message = "Hello world from " + process.name + "!";
            if (inst.printVariable) message = inst.a + std::to_string(valueOf(process, inst.b));
            process.logs.push_back("(" + formatTime(std::chrono::system_clock::now()) + ") Core: " +
                std::to_string(process.lastCore) + " \"" + message + "\"");
            break;
        }
        case Op::Declare:
            process.variables[inst.a] = valueOf(process, inst.b);
            break;
        case Op::Add:
            process.variables[inst.a] = clamp16(static_cast<std::int64_t>(valueOf(process, inst.b)) + valueOf(process, inst.c));
            break;
        case Op::Subtract:
            process.variables[inst.a] = clamp16(static_cast<std::int64_t>(valueOf(process, inst.b)) - valueOf(process, inst.c));
            break;
        case Op::Sleep:
            process.status = Status::Sleeping;
            process.wakeTick = tick_ + static_cast<std::uint64_t>(inst.ticks);
            break;
        case Op::For:
            throw std::logic_error("FOR should be expanded before execution");
    }
    if (process.pc >= process.executedInstructions.size() && process.status == Status::Running) {
        process.status = Status::Finished;
        process.finishedAt = std::chrono::system_clock::now();
    }
}

void Scheduler::assignCore(int core) {
    while (!ready_.empty()) {
        auto p = ready_.front();
        ready_.pop_front();
        if (p->status != Status::Ready) continue;
        p->status = Status::Running;
        p->lastCore = core;
        p->sliceUsed = 0;
        cores_[static_cast<std::size_t>(core)] = std::move(p);
        return;
    }
}

void Scheduler::freeCore(int core) { cores_[static_cast<std::size_t>(core)].reset(); }

void Scheduler::tickLocked() {
    ++tick_;
    if (generating_ && tick_ >= nextGenerationTick_) {
        std::string name;
        do { name = "p" + std::to_string(nextGeneratedName_++); } while (byName_.count(name));
        createLocked(name);
        nextGenerationTick_ = tick_ + static_cast<std::uint64_t>(config_.batchFrequency) * kTicksPerFreqUnit;
    }
    for (auto& p : createdOrder_) {
        if (p->status == Status::Sleeping && p->wakeTick <= tick_) {
            if (p->pc >= p->executedInstructions.size()) {
                p->status = Status::Finished;
                p->finishedAt = std::chrono::system_clock::now();
            } else {
                p->status = Status::Ready;
                ready_.push_back(p);
            }
        }
    }
    for (int core = 0; core < config_.numCpu; ++core) {
        if (!cores_[static_cast<std::size_t>(core)]) assignCore(core);
    }
    for (int core = 0; core < config_.numCpu; ++core) {
        auto p = cores_[static_cast<std::size_t>(core)];
        if (!p) continue;
        if (p->cooldown > 0) {
            --p->cooldown; // busy waiting on the same CPU
        } else {
            executeInstruction(*p);
            // The final instruction does not need a post-instruction delay.
            if (p->status == Status::Running) p->cooldown = config_.delaysPerExec;
        }
        ++p->sliceUsed;
        if (p->status != Status::Running) {
            freeCore(core);
        } else if (config_.algorithm == "rr" && p->sliceUsed >= config_.quantumCycles) {
            p->status = Status::Ready;
            ready_.push_back(p);
            freeCore(core);
        }
    }
    // Refill cores freed this tick (quantum expiry, sleep, finish) so that a
    // snapshot taken between ticks never sees idle cores while work is ready.
    for (int core = 0; core < config_.numCpu; ++core) {
        if (!cores_[static_cast<std::size_t>(core)]) assignCore(core);
    }
}

ProcessSnapshot Scheduler::copyProcess(const Process& p) const {
    ProcessSnapshot copy;
    copy.name = p.name;
    copy.pid = p.pid;
    copy.status = p.status;
    copy.currentInstruction = p.pc;
    copy.totalInstructions = p.executedInstructions.size();
    copy.lastCore = p.lastCore;
    copy.wakeTick = p.wakeTick;
    copy.logs = p.logs;
    copy.variables = p.variables;
    copy.createdAt = p.createdAt;
    copy.finishedAt = p.finishedAt;
    return copy;
}

bool Scheduler::getProcess(const std::string& name, ProcessSnapshot& result) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = byName_.find(name);
    if (it == byName_.end()) return false;
    result = copyProcess(*it->second);
    return true;
}

SystemSnapshot Scheduler::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    SystemSnapshot snap;
    snap.tick = tick_;
    snap.cpuCount = config_.numCpu;
    snap.generatorActive = generating_;
    for (const auto& p : cores_) if (p) ++snap.coresUsed;
    snap.processes.reserve(createdOrder_.size());
    for (const auto& p : createdOrder_) snap.processes.push_back(copyProcess(*p));
    return snap;
}

void Scheduler::startGenerator() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!generating_) {
        generating_ = true;
        nextGenerationTick_ = tick_ + static_cast<std::uint64_t>(config_.batchFrequency) * kTicksPerFreqUnit;
    }
}

void Scheduler::stopGenerator() {
    std::lock_guard<std::mutex> lock(mutex_);
    generating_ = false;
}

std::string processReport(const ProcessSnapshot& p) {
    std::ostringstream out;
    out << "Process name: " << p.name << "\n"
        << "ID: " << p.pid << "\n"
        << "Status: " << statusText(p.status) << "\n"
        << "Logs:\n";
    if (p.logs.empty()) out << "  (No PRINT instructions executed yet.)\n";
    else for (const auto& log : p.logs) out << log << '\n';
    if (p.status == Status::Finished) out << "Finished!\n";
    else {
        out << "Current instruction line: " << p.currentInstruction << '\n'
            << "Lines of code: " << p.totalInstructions << '\n';
        if (p.status == Status::Sleeping) out << "Sleeping until tick: " << p.wakeTick << '\n';
    }
    return out.str();
}

std::string systemReport(const SystemSnapshot& snap) {
    std::ostringstream out;
    const double percent = snap.cpuCount > 0 ? (100.0 * snap.coresUsed / snap.cpuCount) : 0;
    out << std::fixed << std::setprecision(1)
        << "CPU utilization: " << percent << "%\n"
        << "Cores used: " << snap.coresUsed << '\n'
        << "Cores available: " << (snap.cpuCount - snap.coresUsed) << '\n'
        << "CPU tick: " << snap.tick << '\n'
        << "Process generator: " << (snap.generatorActive ? "ON" : "OFF") << "\n"
        << "-----------------------------------------------\n"
        << "Running processes:\n";
    bool hasActive = false;
    for (const auto& p : snap.processes) {
        // Only processes currently on a core; READY/SLEEPING ones are queued, not running.
        if (p.status != Status::Running) continue;
        hasActive = true;
        out << std::left << std::setw(18) << p.name << " (" << formatTime(p.createdAt) << ")  "
            << std::setw(10) << statusText(p.status) << " Core: ";
        if (p.lastCore < 0 || p.status != Status::Running) out << "-";
        else out << p.lastCore;
        out << "  " << p.currentInstruction << " / " << p.totalInstructions << '\n';
    }
    if (!hasActive) out << "  (none)\n";
    out << "\nFinished processes:\n";
    bool hasFinished = false;
    for (const auto& p : snap.processes) {
        if (p.status != Status::Finished) continue;
        hasFinished = true;
        out << std::left << std::setw(18) << p.name << " (" << formatTime(p.createdAt) << ")  "
            << "Finished  " << p.currentInstruction << " / " << p.totalInstructions << '\n';
    }
    if (!hasFinished) out << "  (none)\n";
    out << "-----------------------------------------------\n";
    return out.str();
}

} // namespace csopesy
