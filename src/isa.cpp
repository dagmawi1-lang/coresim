#include "isa.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace coresim {

static const OpInfo TABLE[] = {
  {Op::NOP, "NOP", Form::None, "do nothing"},
  {Op::HALT, "HALT", Form::None, "stop the CPU"},
  {Op::LDI, "LDI", Form::RImm, "rd = imm"},
  {Op::MOV, "MOV", Form::RR, "rd = rs"},
  {Op::LOAD, "LOAD", Form::RMem, "rd = mem[rs + imm]"},
  {Op::STORE, "STORE", Form::MemR, "mem[rd + imm] = rs"},
  {Op::PUSH, "PUSH", Form::R, "SP -= 1; mem[SP] = rd"},
  {Op::POP, "POP", Form::R, "rd = mem[SP]; SP += 1"},
  {Op::ADD, "ADD", Form::RR, "rd = rd + rs (sets Z N C V)"},
  {Op::SUB, "SUB", Form::RR, "rd = rd - rs (sets Z N C V)"},
  {Op::MUL, "MUL", Form::RR, "rd = low16(rd * rs) (C = overflow)"},
  {Op::AND, "AND", Form::RR, "rd = rd & rs"},
  {Op::OR, "OR", Form::RR, "rd = rd | rs"},
  {Op::XOR, "XOR", Form::RR, "rd = rd ^ rs"},
  {Op::NOT, "NOT", Form::R, "rd = ~rd"},
  {Op::ADDI, "ADDI", Form::RImm, "rd = rd + imm"},
  {Op::SHL, "SHL", Form::RImm, "rd = rd << imm (C = last bit out)"},
  {Op::SHR, "SHR", Form::RImm, "rd = rd >> imm, logical (C = last bit out)"},
  {Op::CMP, "CMP", Form::RR, "flags from rd - rs"},
  {Op::CMPI, "CMPI", Form::RImm, "flags from rd - imm"},
  {Op::JMP, "JMP", Form::Addr, "PC = addr"},
  {Op::JZ, "JZ", Form::Addr, "jump if Z"},
  {Op::JNZ, "JNZ", Form::Addr, "jump if !Z"},
  {Op::JN, "JN", Form::Addr, "jump if N != V (signed less-than after CMP)"},
  {Op::JC, "JC", Form::Addr, "jump if C (unsigned borrow/carry)"},
  {Op::JGE, "JGE", Form::Addr, "jump if N == V (signed greater-or-equal)"},
  {Op::CALL, "CALL", Form::Addr, "push return address; PC = addr"},
  {Op::RET, "RET", Form::None, "PC = pop()"},
  {Op::OUT, "OUT", Form::R, "append rd to the output port"},
};

const OpInfo* all_ops(size_t& count) { count = sizeof(TABLE) / sizeof(TABLE[0]); return TABLE; }
const OpInfo* info(Op op) { for (const auto& i : TABLE) if (i.op == op) return &i; return nullptr; }
const OpInfo* info(const std::string& m) {
  std::string u = m; std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c) { return std::toupper(c); });
  for (const auto& i : TABLE) if (u == i.name) return &i;
  return nullptr;
}

std::string disassemble(const Instr& in) {
  const OpInfo* i = info(in.op);
  if (!i) { std::ostringstream o; o << ".word 0x" << std::hex << in.hi() << ", 0x" << in.imm; return o.str(); }
  std::ostringstream o; o << i->name;
  const int simm = static_cast<int16_t>(in.imm);
  switch (i->form) {
    case Form::None: break;
    case Form::R: o << " R" << int(in.rd); break;
    case Form::RR: o << " R" << int(in.rd) << ", R" << int(in.rs); break;
    case Form::RImm: o << " R" << int(in.rd) << ", " << simm; break;
    case Form::RMem: o << " R" << int(in.rd) << ", [R" << int(in.rs) << (simm ? (simm > 0 ? "+" : "") + std::to_string(simm) : "") << "]"; break;
    case Form::MemR: o << " [R" << int(in.rd) << (simm ? (simm > 0 ? "+" : "") + std::to_string(simm) : "") << "], R" << int(in.rs); break;
    case Form::Addr: o << " 0x" << std::hex << in.imm; break;
  }
  return o.str();
}

}  // namespace coresim
