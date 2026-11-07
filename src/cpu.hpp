#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "isa.hpp"

namespace coresim {

struct Flags { bool z = false, n = false, c = false, v = false; };

class CpuError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

// Snapshot of architectural state, used for tracing and visualization.
struct State {
  std::array<uint16_t, 8> r{}; uint16_t pc = 0, sp = 0; Flags f; uint64_t cycles = 0; bool halted = false;
};

struct TraceEntry { State before; Instr instr; std::string text; State after; std::vector<std::pair<uint16_t, uint16_t>> memWrites; };

class Cpu {
 public:
  static constexpr size_t MEM_WORDS = 65536;
  static constexpr uint16_t STACK_TOP = 0xFF00;  // stack grows down from here

  Cpu();
  void reset();
  void load(const std::vector<uint16_t>& image, uint16_t at = 0);
  TraceEntry step();                    // execute one instruction; throws CpuError on faults
  uint64_t run(uint64_t maxSteps = 1'000'000, std::vector<TraceEntry>* trace = nullptr);

  const State& state() const { return s_; }
  uint16_t reg(int i) const { return s_.r.at(static_cast<size_t>(i)); }
  uint16_t mem(uint16_t addr) const { return m_[addr]; }
  void setMem(uint16_t addr, uint16_t v) { m_[addr] = v; }
  const std::vector<uint16_t>& output() const { return out_; }
  Instr fetchAt(uint16_t pc) const { return Instr::decode(m_[pc], m_[static_cast<uint16_t>(pc + 1)]); }

 private:
  void setNZ(uint16_t v);
  void push(uint16_t v, TraceEntry& t);
  uint16_t pop();
  std::vector<uint16_t> m_;
  State s_;
  std::vector<uint16_t> out_;
};

}  // namespace coresim
