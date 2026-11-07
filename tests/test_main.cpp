// Minimal test runner (no external frameworks so it builds anywhere).
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../src/assembler.hpp"
#include "../src/cpu.hpp"

using namespace coresim;
static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; } } while (0)
#define CHECK_THROWS(expr, Type) do { ++checks; bool t = false; try { expr; } catch (const Type&) { t = true; } if (!t) { ++failures; std::cerr << "  FAIL " << __LINE__ << " expected " #Type "\n"; } } while (0)

static Cpu runSrc(const std::string& src, uint64_t max = 100000) { Cpu c; c.load(assemble(src).image); c.run(max); return c; }
static std::string file(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::vector<uint16_t> out(const std::string& prog) { return runSrc(file(std::string(PROGRAMS_DIR) + "/" + prog)).output(); }

int main() {
  std::vector<std::pair<std::string, std::function<void()>>> tests = {
    {"encoding round-trip", [] { Instr i{Op::LOAD, 3, 5, 0xBEEF}; Instr d = Instr::decode(i.hi(), i.imm); CHECK(d.op == Op::LOAD && d.rd == 3 && d.rs == 5 && d.imm == 0xBEEF); }},
    {"ADD sets carry and zero", [] { Cpu c = runSrc("LDI R1, 0xFFFF\nLDI R2, 1\nADD R1, R2\nHALT"); CHECK(c.reg(1) == 0); CHECK(c.state().f.z); CHECK(c.state().f.c); CHECK(!c.state().f.v); }},
    {"signed overflow flag", [] { Cpu c = runSrc("LDI R1, 0x7FFF\nADDI R1, 1\nHALT"); CHECK(c.reg(1) == 0x8000); CHECK(c.state().f.v); CHECK(c.state().f.n); CHECK(!c.state().f.c); }},
    {"SUB borrow", [] { Cpu c = runSrc("LDI R1, 3\nLDI R2, 5\nSUB R1, R2\nHALT"); CHECK(c.reg(1) == 0xFFFE); CHECK(c.state().f.c); CHECK(c.state().f.n); }},
    {"shifts and logic", [] { Cpu c = runSrc("LDI R1, 0x8001\nSHL R1, 1\nLDI R2, 0x0F0F\nLDI R3, 0x00FF\nAND R2, R3\nNOT R3\nHALT"); CHECK(c.reg(1) == 2); CHECK(c.reg(2) == 0x000F); CHECK(c.reg(3) == 0xFF00); }},
    {"signed compare branches", [] { Cpu c = runSrc("LDI R1, -5\nCMPI R1, 3\nJN less\nLDI R0, 1\nHALT\nless: LDI R0, 2\nHALT"); CHECK(c.reg(0) == 2); }},
    {"memory load/store with offsets", [] { Cpu c = runSrc("LDI R1, buf\nLDI R2, 77\nSTORE [R1+2], R2\nLOAD R3, [R1+2]\nHALT\nbuf: .space 4"); CHECK(c.reg(3) == 77); }},
    {"stack push/pop order", [] { Cpu c = runSrc("LDI R1, 1\nLDI R2, 2\nPUSH R1\nPUSH R2\nPOP R3\nPOP R4\nHALT"); CHECK(c.reg(3) == 2 && c.reg(4) == 1); CHECK(c.state().sp == Cpu::STACK_TOP); }},
    {"program: fibonacci", [] { auto o = out("fibonacci.asm"); CHECK((o == std::vector<uint16_t>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34})); }},
    {"program: sum array", [] { CHECK(out("sum_array.asm") == std::vector<uint16_t>{23}); }},
    {"program: recursive factorial", [] { CHECK(out("factorial.asm") == std::vector<uint16_t>{720}); }},
    {"program: bubble sort (signed)", [] { auto o = out("bubble_sort.asm"); std::vector<int16_t> s(o.begin(), o.end()); CHECK((s == std::vector<int16_t>{-3, 0, 7, 7, 19, 42, 100})); }},
    {"program: hello", [] { CHECK((out("hello.asm") == std::vector<uint16_t>{72, 69, 76, 76, 79})); }},
    {"assembler errors carry line numbers", [] {
      try { assemble("NOP\nLDI R9, 1"); CHECK(false); } catch (const AsmError& e) { CHECK(e.line == 2); }
      CHECK_THROWS(assemble("FOO R1"), AsmError);
      CHECK_THROWS(assemble("JMP nowhere"), AsmError);
      CHECK_THROWS(assemble("a: NOP\na: NOP"), AsmError);
      CHECK_THROWS(assemble("LDI R1, 70000"), AsmError);
      CHECK_THROWS(assemble("ADD R1"), AsmError);
      CHECK_THROWS(assemble("LOAD R1, R2"), AsmError);
    }},
    {"comments, char literals, hex, labels on own line", [] { Cpu c = runSrc("start:\n  LDI R1, 'A' ; comment\n  LDI R2, 0x10 # also comment\n  HALT"); CHECK(c.reg(1) == 65 && c.reg(2) == 16); }},
    {"faults: infinite loop, stack underflow, illegal opcode", [] {
      CHECK_THROWS(runSrc("x: JMP x", 1000), CpuError);
      CHECK_THROWS(runSrc("RET"), CpuError);
      Cpu c; c.load({0xEE00, 0}); CHECK_THROWS(c.step(), CpuError);
      Cpu h = runSrc("HALT"); CHECK_THROWS(h.step(), CpuError);
    }},
    {"trace records memory writes and cycles", [] {
      Cpu c; c.load(assemble("LDI R1, 100\nLDI R2, 5\nSTORE [R1], R2\nHALT").image); std::vector<TraceEntry> t; c.run(100, &t);
      CHECK(t.size() == 4); CHECK(t[2].memWrites.size() == 1 && t[2].memWrites[0].first == 100 && t[2].memWrites[0].second == 5); CHECK(c.state().cycles == 4);
    }},
  };
  for (auto& [name, fn] : tests) { int before = failures; fn(); std::cout << (failures == before ? "PASS " : "FAIL ") << name << "\n"; }
  std::cout << "\n" << tests.size() << " tests, " << checks << " checks, " << failures << " failures\n";
  return failures ? 1 : 0;
}
