#include "scheduler.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace csopesy;

Instruction print() { Instruction x; x.op = Op::Print; return x; }
Instruction decl(const char* name, const char* val) { Instruction x; x.op = Op::Declare; x.a=name; x.b=val; return x; }
Instruction calc(Op op, const char* out, const char* lhs, const char* rhs) {
    Instruction x; x.op=op; x.a=out; x.b=lhs; x.c=rhs; return x;
}
Instruction sleepTicks(int ticks) { Instruction x; x.op=Op::Sleep; x.ticks=ticks; return x; }
Instruction loop(int reps, std::vector<Instruction> body) {
    Instruction x; x.op=Op::For; x.repeats=reps; x.body=std::move(body); return x;
}

void testConfig() {
    Config cfg; std::string error;
    assert(Config::load("config-demo.txt", cfg, error));
    assert(cfg.numCpu == 4 && cfg.algorithm == "rr" && cfg.minInstructions == 12);
    { std::ofstream f("test-invalid-config.tmp"); f << "num-cpu 0\n"; }
    assert(!Config::load("test-invalid-config.tmp", cfg, error));
    std::remove("test-invalid-config.tmp");
}

void testNestedLoopAndArithmetic() {
    Config cfg; cfg.numCpu = 1; cfg.algorithm = "fcfs";
    Scheduler scheduler(cfg);
    Instruction echo; echo.op=Op::Print; echo.printVariable=true; echo.a="Value from x: "; echo.b="x";
    const auto nested = loop(2, {loop(3, {calc(Op::Add,"x","x","1")})});
    scheduler.createProgramForTest("math", {
        decl("x", "65535"), calc(Op::Add, "x", "x", "5"),
        calc(Op::Subtract, "x", "0", "9"), nested,
        calc(Op::Add, "result", "undeclared", "5"), echo, print()
    });
    scheduler.advanceForTest(30);
    ProcessSnapshot snap;
    assert(scheduler.getProcess("math", snap));
    assert(snap.status == Status::Finished);
    assert(snap.totalInstructions == 12); // DECLARE + ADD + SUB + six loop additions + ADD + PRINT + PRINT
    assert(snap.currentInstruction == 12);
    assert(snap.variables.at("x") == 6); // 0 clamped, then +6
    assert(snap.variables.at("undeclared") == 0);
    assert(snap.variables.at("result") == 5);
    assert(snap.logs.size() == 2);
    assert(snap.logs.front().find("Value from x: 6") != std::string::npos);
    assert(snap.logs.back().find("Hello world from math!") != std::string::npos);
}

void testSleepFreesCore() {
    Config cfg; cfg.numCpu = 1; cfg.algorithm = "fcfs";
    Scheduler scheduler(cfg);
    scheduler.createProgramForTest("sleeper", {sleepTicks(4), print()});
    scheduler.createProgramForTest("next", {print(), print()});
    scheduler.advanceForTest(1);
    ProcessSnapshot a,b;
    assert(scheduler.getProcess("sleeper", a) && a.status == Status::Sleeping);
    assert(scheduler.snapshot().coresUsed == 0);
    scheduler.advanceForTest(2);
    assert(scheduler.getProcess("next", b) && b.status == Status::Finished);
    scheduler.advanceForTest(5);
    assert(scheduler.getProcess("sleeper", a) && a.status == Status::Finished);
}

void testRoundRobinPreemption() {
    Config cfg; cfg.numCpu = 1; cfg.algorithm = "rr"; cfg.quantumCycles = 1;
    Scheduler scheduler(cfg);
    scheduler.createProgramForTest("first", {print(), print(), print()});
    scheduler.createProgramForTest("second", {print(), print(), print()});
    scheduler.advanceForTest(2);
    ProcessSnapshot a,b;
    assert(scheduler.getProcess("first", a) && a.currentInstruction == 1);
    assert(scheduler.getProcess("second", b) && b.currentInstruction == 1);
    scheduler.advanceForTest(10);
    assert(scheduler.getProcess("first", a) && a.status == Status::Finished);
    assert(scheduler.getProcess("second", b) && b.status == Status::Finished);
}

void testDelayAndParallelCores() {
    Config cfg; cfg.numCpu = 2; cfg.algorithm = "fcfs"; cfg.delaysPerExec = 2;
    Scheduler scheduler(cfg);
    scheduler.createProgramForTest("alpha", {print(), print()});
    scheduler.createProgramForTest("beta", {print(), print()});
    scheduler.advanceForTest(1);
    assert(scheduler.snapshot().coresUsed == 2);
    scheduler.advanceForTest(2);
    ProcessSnapshot a;
    assert(scheduler.getProcess("alpha", a) && a.currentInstruction == 1);
    scheduler.advanceForTest(1);
    assert(scheduler.getProcess("alpha", a) && a.status == Status::Finished);
}

void testGeneratorStop() {
    Config cfg; cfg.numCpu = 2; cfg.algorithm = "rr"; cfg.quantumCycles=2;
    cfg.batchFrequency = 3; cfg.minInstructions=4; cfg.maxInstructions=4;
    Scheduler scheduler(cfg);
    scheduler.startGenerator();
    scheduler.advanceForTest(10);
    auto snap = scheduler.snapshot();
    assert(snap.processes.size() == 3);
    assert(snap.processes[0].name == "p1" && snap.processes[2].name == "p3");
    scheduler.stopGenerator();
    scheduler.advanceForTest(20);
    assert(scheduler.snapshot().processes.size() == 3);
    const auto report = systemReport(scheduler.snapshot());
    assert(report.find("Finished processes:") != std::string::npos);
    assert(report.find("CPU utilization:") != std::string::npos);
}

void testRandomInstructionBudget() {
    Config cfg; cfg.numCpu=4; cfg.minInstructions=35; cfg.maxInstructions=45;
    Scheduler scheduler(cfg);
    for (int i=0; i<100; ++i) {
        std::string err;
        assert(scheduler.createProcess("process_"+std::to_string(i), err));
    }
    for (const auto& p : scheduler.snapshot().processes)
        assert(p.totalInstructions >= 35 && p.totalInstructions <= 45);
    scheduler.advanceForTest(3000);
    for (const auto& p : scheduler.snapshot().processes) assert(p.status == Status::Finished);
}

int main() {
    testConfig();
    testNestedLoopAndArithmetic();
    testSleepFreesCore();
    testRoundRobinPreemption();
    testDelayAndParallelCores();
    testGeneratorStop();
    testRandomInstructionBudget();
    std::cout << "PASS: config, uint16 arithmetic, PRINT variable, nested FOR, SLEEP, "
                 "FCFS, RR, multi-core, execution delay, batch generation, reporting.\n";
}
