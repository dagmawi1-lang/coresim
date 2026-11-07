#include "assembler.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include "isa.hpp"

namespace coresim {
namespace {

std::string trim(std::string s) {
  auto ns = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), ns));
  s.erase(std::find_if(s.rbegin(), s.rend(), ns).base(), s.end());
  return s;
}
std::string upper(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); }); return s; }

std::vector<std::string> splitOperands(const std::string& s) {
  std::vector<std::string> out; std::string cur; int depth = 0; bool quote = false;
  for (char c : s) {
    if (c == '\'') quote = !quote;
    if (!quote && c == '[') ++depth;
    if (!quote && c == ']') --depth;
    if (c == ',' && depth == 0 && !quote) { out.push_back(trim(cur)); cur.clear(); } else cur += c;
  }
  if (!trim(cur).empty()) out.push_back(trim(cur));
  return out;
}

struct Line { int no; std::string label, mnem; std::vector<std::string> ops; };

int parseReg(const std::string& t, int line) {
  static const std::regex re(R"(^[Rr]([0-7])$)");
  std::smatch m;
  if (!std::regex_match(t, m, re)) throw AsmError(line, "expected a register R0–R7, got '" + t + "'");
  return std::stoi(m[1]);
}

long parseValue(const std::string& raw, const std::map<std::string, uint16_t>& syms, int line, bool allowUnknown) {
  std::string t = trim(raw);
  if (t.empty()) throw AsmError(line, "missing value");
  if (t.size() == 3 && t.front() == '\'' && t.back() == '\'') return static_cast<unsigned char>(t[1]);
  bool neg = false;
  if (t[0] == '-') { neg = true; t = trim(t.substr(1)); }
  long v;
  try {
    size_t used = 0;
    if (t.rfind("0x", 0) == 0 || t.rfind("0X", 0) == 0) v = std::stol(t.substr(2), &used, 16), used += 2;
    else if (t.rfind("0b", 0) == 0) v = std::stol(t.substr(2), &used, 2), used += 2;
    else if (std::isdigit(static_cast<unsigned char>(t[0]))) v = std::stol(t, &used, 10);
    else {
      auto it = syms.find(t);
      if (it == syms.end()) { if (allowUnknown) return 0; throw AsmError(line, "unknown label or constant '" + t + "'"); }
      return neg ? -long(it->second) : it->second;
    }
    if (used != t.size()) throw AsmError(line, "bad number '" + raw + "'");
  } catch (const std::invalid_argument&) { throw AsmError(line, "bad number '" + raw + "'"); }
  catch (const std::out_of_range&) { throw AsmError(line, "number out of range '" + raw + "'"); }
  return neg ? -v : v;
}

uint16_t imm16(long v, int line) {
  if (v < -32768 || v > 65535) throw AsmError(line, "value " + std::to_string(v) + " does not fit in 16 bits");
  return static_cast<uint16_t>(v);
}

// "[R2+3]" -> (reg, offset)
std::pair<int, long> parseMem(const std::string& t, const std::map<std::string, uint16_t>& syms, int line, bool allowUnknown) {
  static const std::regex re(R"(^\[\s*([Rr][0-7])\s*(?:([+-])\s*([^\]]+?))?\s*\]$)");
  std::smatch m;
  if (!std::regex_match(t, m, re)) throw AsmError(line, "expected memory operand like [R1] or [R1+4], got '" + t + "'");
  long off = 0;
  if (m[3].matched) { off = parseValue(m[3], syms, line, allowUnknown); if (m[2] == "-") off = -off; }
  return {parseReg(m[1], line), off};
}

}  // namespace

Program assemble(const std::string& source) {
  Program p;
  std::vector<Line> lines;
  std::map<std::string, uint16_t> syms;
  std::istringstream in(source);
  std::string raw; int no = 0;
  static const std::regex labelRe(R"(^([A-Za-z_][A-Za-z0-9_]*)\s*:\s*(.*)$)");
  while (std::getline(in, raw)) {
    ++no; p.source.push_back(raw);
    std::string s = raw;
    bool quote = false;
    for (size_t i = 0; i < s.size(); ++i) { if (s[i] == '\'') quote = !quote; if (!quote && (s[i] == ';' || s[i] == '#')) { s = s.substr(0, i); break; } }
    s = trim(s);
    Line L{no, "", "", {}};
    std::smatch m;
    if (std::regex_match(s, m, labelRe)) { L.label = m[1]; s = trim(m[2]); }
    if (!s.empty()) {
      auto sp = s.find_first_of(" \t");
      L.mnem = upper(s.substr(0, sp));
      if (sp != std::string::npos) L.ops = splitOperands(s.substr(sp + 1));
    }
    if (!L.label.empty() || !L.mnem.empty()) lines.push_back(L);
  }
  // Pass 1: addresses for labels; .equ constants.
  uint16_t addr = 0;
  for (auto& L : lines) {
    if (!L.label.empty()) {
      if (syms.count(L.label)) throw AsmError(L.no, "duplicate label '" + L.label + "'");
      if (info(L.label) || std::regex_match(L.label, std::regex(R"([Rr][0-7])"))) throw AsmError(L.no, "label '" + L.label + "' is a reserved word");
      syms[L.label] = addr;
    }
    if (L.mnem.empty()) continue;
    if (L.mnem == ".EQU") {
      if (L.ops.size() != 2) throw AsmError(L.no, ".equ needs NAME, value");
      syms[L.ops[0]] = imm16(parseValue(L.ops[1], syms, L.no, false), L.no);
    } else if (L.mnem == ".WORD") { addr = static_cast<uint16_t>(addr + L.ops.size()); }
    else if (L.mnem == ".SPACE") { addr = static_cast<uint16_t>(addr + parseValue(L.ops.at(0), syms, L.no, false)); }
    else if (info(L.mnem)) { addr = static_cast<uint16_t>(addr + 2); }
    else throw AsmError(L.no, "unknown instruction '" + L.mnem + "'");
  }
  // Pass 2: encode.
  auto emit = [&](uint16_t w) { p.image.push_back(w); };
  for (auto& L : lines) {
    if (L.mnem.empty() || L.mnem == ".EQU") continue;
    if (L.mnem == ".WORD") { for (auto& o : L.ops) emit(imm16(parseValue(o, syms, L.no, false), L.no)); continue; }
    if (L.mnem == ".SPACE") { for (long i = parseValue(L.ops[0], syms, L.no, false); i > 0; --i) emit(0); continue; }
    const OpInfo* oi = info(L.mnem);
    Instr ins; ins.op = oi->op;
    auto need = [&](size_t n) { if (L.ops.size() != n) throw AsmError(L.no, std::string(oi->name) + " expects " + std::to_string(n) + " operand(s), got " + std::to_string(L.ops.size())); };
    switch (oi->form) {
      case Form::None: need(0); break;
      case Form::R: need(1); ins.rd = parseReg(L.ops[0], L.no); break;
      case Form::RR: need(2); ins.rd = parseReg(L.ops[0], L.no); ins.rs = parseReg(L.ops[1], L.no); break;
      case Form::RImm: need(2); ins.rd = parseReg(L.ops[0], L.no); ins.imm = imm16(parseValue(L.ops[1], syms, L.no, false), L.no); break;
      case Form::RMem: { need(2); ins.rd = parseReg(L.ops[0], L.no); auto [r, off] = parseMem(L.ops[1], syms, L.no, false); ins.rs = r; ins.imm = imm16(off, L.no); break; }
      case Form::MemR: { need(2); auto [r, off] = parseMem(L.ops[0], syms, L.no, false); ins.rd = r; ins.imm = imm16(off, L.no); ins.rs = parseReg(L.ops[1], L.no); break; }
      case Form::Addr: need(1); ins.imm = imm16(parseValue(L.ops[0], syms, L.no, false), L.no); break;
    }
    p.sourceLine[static_cast<uint16_t>(p.image.size())] = L.no;
    emit(ins.hi()); emit(ins.imm);
  }
  p.labels = syms;
  return p;
}

}  // namespace coresim
