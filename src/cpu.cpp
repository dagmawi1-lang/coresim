#include "cpu.hpp"
#include <sstream>

namespace coresim {

Cpu::Cpu() : m_(MEM_WORDS, 0) { reset(); }

void Cpu::reset() { s_ = State{}; s_.sp = STACK_TOP; out_.clear(); }

void Cpu::load(const std::vector<uint16_t>& image, uint16_t at) {
  if (image.size() + at > MEM_WORDS) throw CpuError("program does not fit in memory");
  for (size_t i = 0; i < image.size(); ++i) m_[at + i] = image[i];
}

void Cpu::setNZ(uint16_t v) { s_.f.z = v == 0; s_.f.n = (v & 0x8000) != 0; }

void Cpu::push(uint16_t v, TraceEntry& t) {
  if (s_.sp <= 0xF000) throw CpuError("stack overflow (SP reached 0xF000)");
  s_.sp = static_cast<uint16_t>(s_.sp - 1);
  m_[s_.sp] = v; t.memWrites.push_back({s_.sp, v});
}
uint16_t Cpu::pop() {
  if (s_.sp >= STACK_TOP) throw CpuError("stack underflow (POP/RET with empty stack)");
  return m_[s_.sp++];
}

TraceEntry Cpu::step() {
  if (s_.halted) throw CpuError("CPU is halted");
  TraceEntry t; t.before = s_;
  const uint16_t pc = s_.pc;
  const Instr in = fetchAt(pc);
  t.instr = in; t.text = disassemble(in);
  if (!info(in.op)) { std::ostringstream o; o << "illegal opcode 0x" << std::hex << int(static_cast<uint8_t>(in.op)) << " at 0x" << pc; throw CpuError(o.str()); }
  if (in.rd > 7 || in.rs > 7) throw CpuError("register index out of range");
  s_.pc = static_cast<uint16_t>(pc + 2);
  uint16_t& rd = s_.r[in.rd];
  const uint16_t rs = s_.r[in.rs];
  auto addFlags = [&](uint16_t a, uint16_t b, bool sub) {
    const uint32_t res = sub ? uint32_t(a) - uint32_t(b) : uint32_t(a) + uint32_t(b);
    const uint16_t r16 = static_cast<uint16_t>(res);
    setNZ(r16);
    s_.f.c = sub ? (a < b) : (res > 0xFFFF);  // carry for add, borrow for sub
    const bool sa = a & 0x8000, sb = b & 0x8000, sr = r16 & 0x8000;
    s_.f.v = sub ? (sa != sb && sr != sa) : (sa == sb && sr != sa);
    return r16;
  };
  switch (in.op) {
    case Op::NOP: break;
    case Op::HALT: s_.halted = true; break;
    case Op::LDI: rd = in.imm; break;
    case Op::MOV: rd = rs; break;
    case Op::LOAD: rd = m_[static_cast<uint16_t>(rs + in.imm)]; break;
    case Op::STORE: { uint16_t a = static_cast<uint16_t>(s_.r[in.rd] + in.imm); m_[a] = rs; t.memWrites.push_back({a, rs}); break; }
    case Op::PUSH: push(rd, t); break;
    case Op::POP: rd = pop(); break;
    case Op::ADD: rd = addFlags(rd, rs, false); break;
    case Op::SUB: rd = addFlags(rd, rs, true); break;
    case Op::ADDI: rd = addFlags(rd, in.imm, false); break;
    case Op::MUL: { uint32_t p = uint32_t(rd) * uint32_t(rs); rd = static_cast<uint16_t>(p); setNZ(rd); s_.f.c = p > 0xFFFF; s_.f.v = false; break; }
    case Op::AND: rd &= rs; setNZ(rd); s_.f.c = s_.f.v = false; break;
    case Op::OR: rd |= rs; setNZ(rd); s_.f.c = s_.f.v = false; break;
    case Op::XOR: rd ^= rs; setNZ(rd); s_.f.c = s_.f.v = false; break;
    case Op::NOT: rd = static_cast<uint16_t>(~rd); setNZ(rd); break;
    case Op::SHL: { unsigned n = in.imm & 0xF; if (n) { s_.f.c = (rd >> (16 - n)) & 1; rd = static_cast<uint16_t>(rd << n); } setNZ(rd); break; }
    case Op::SHR: { unsigned n = in.imm & 0xF; if (n) { s_.f.c = (rd >> (n - 1)) & 1; rd = static_cast<uint16_t>(rd >> n); } setNZ(rd); break; }
    case Op::CMP: addFlags(rd, rs, true); break;
    case Op::CMPI: addFlags(rd, in.imm, true); break;
    case Op::JMP: s_.pc = in.imm; break;
    case Op::JZ: if (s_.f.z) s_.pc = in.imm; break;
    case Op::JNZ: if (!s_.f.z) s_.pc = in.imm; break;
    case Op::JN: if (s_.f.n != s_.f.v) s_.pc = in.imm; break;
    case Op::JGE: if (s_.f.n == s_.f.v) s_.pc = in.imm; break;
    case Op::JC: if (s_.f.c) s_.pc = in.imm; break;
    case Op::CALL: push(s_.pc, t); s_.pc = in.imm; break;
    case Op::RET: s_.pc = pop(); break;
    case Op::OUT: out_.push_back(rd); break;
  }
  ++s_.cycles;
  t.after = s_;
  return t;
}

uint64_t Cpu::run(uint64_t maxSteps, std::vector<TraceEntry>* trace) {
  uint64_t n = 0;
  while (!s_.halted) {
    if (n >= maxSteps) throw CpuError("step limit reached (infinite loop?) after " + std::to_string(n) + " instructions");
    TraceEntry t = step();
    if (trace) trace->push_back(std::move(t));
    ++n;
  }
  return n;
}

}  // namespace coresim
