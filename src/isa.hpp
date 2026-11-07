// CS16 instruction set: a small 16-bit load/store CPU for learning computer architecture.
#pragma once
#include <cstdint>
#include <string>

namespace coresim {

enum class Op : uint8_t {
  NOP = 0x00, HALT = 0x01,
  LDI = 0x10, MOV = 0x11, LOAD = 0x12, STORE = 0x13, PUSH = 0x14, POP = 0x15,
  ADD = 0x20, SUB = 0x21, MUL = 0x22, AND = 0x23, OR = 0x24, XOR = 0x25, NOT = 0x26,
  ADDI = 0x27, SHL = 0x28, SHR = 0x29, CMP = 0x2A, CMPI = 0x2B,
  JMP = 0x30, JZ = 0x31, JNZ = 0x32, JN = 0x33, JC = 0x34, JGE = 0x35, CALL = 0x36, RET = 0x37,
  OUT = 0x40,
};

// Operand shapes, used by the assembler, disassembler, and docs.
enum class Form { None, R, RR, RImm, RMem /* rd, [rs+imm] */, MemR /* [rd+imm], rs */, Addr };

struct OpInfo { Op op; const char* name; Form form; const char* summary; };

// Encoding (32 bits = two 16-bit memory words):  [ opcode:8 | rd:4 | rs:4 ][ imm:16 ]
struct Instr {
  Op op = Op::NOP; uint8_t rd = 0, rs = 0; uint16_t imm = 0;
  uint16_t hi() const { return static_cast<uint16_t>((static_cast<uint16_t>(op) << 8) | ((rd & 0xF) << 4) | (rs & 0xF)); }
  static Instr decode(uint16_t hi, uint16_t lo) { return Instr{static_cast<Op>(hi >> 8), static_cast<uint8_t>((hi >> 4) & 0xF), static_cast<uint8_t>(hi & 0xF), lo}; }
};

const OpInfo* info(Op op);
const OpInfo* info(const std::string& mnemonic);
const OpInfo* all_ops(size_t& count);
std::string disassemble(const Instr& in);

}  // namespace coresim
