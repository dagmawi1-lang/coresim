#pragma once
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace coresim {

struct AsmError : std::runtime_error {
  int line;
  AsmError(int l, const std::string& m) : std::runtime_error("line " + std::to_string(l) + ": " + m), line(l) {}
};

struct Program {
  std::vector<uint16_t> image;                 // memory words starting at address 0
  std::map<std::string, uint16_t> labels;
  std::map<uint16_t, int> sourceLine;          // instruction address -> source line (for tracing)
  std::vector<std::string> source;
};

// Two-pass assembler. Syntax:
//   label:   MNEMONIC operands   ; comment
//   LDI R1, 42 | LDI R1, 0x2A | LDI R1, 'A' | LDI R1, label
//   LOAD R1, [R2+3]   STORE [R2-1], R1
//   .word 1, 2, 3     .space 10     .equ NAME, value
Program assemble(const std::string& source);

}  // namespace coresim
