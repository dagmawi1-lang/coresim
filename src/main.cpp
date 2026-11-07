#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "assembler.hpp"
#include "cpu.hpp"
#include "view.hpp"

using namespace coresim;

static std::string readFile(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("cannot open " + path);
  std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

static int usage() {
  std::cerr << "CoreSim — CS16 CPU instruction simulator\n\n"
               "usage:\n  coresim run <prog.asm> [--trace] [--max N] [--html trace.html] [--no-color]\n"
               "  coresim step <prog.asm>          interactive: Enter=step, c=continue, m ADDR [N]=memory, q=quit\n"
               "  coresim asm <prog.asm>           show the assembled listing (address, machine code, disassembly)\n"
               "  coresim isa                      print the instruction set\n";
  return 2;
}

int main(int argc, char** argv) {
  if (argc < 2) return usage();
  const std::string cmd = argv[1];
  try {
    if (cmd == "isa") {
      size_t n; const OpInfo* ops = all_ops(n);
      std::cout << "Opcode  Mnemonic  Operands        Meaning\n";
      const char* forms[] = {"", "Rd", "Rd, Rs", "Rd, imm", "Rd, [Rs+imm]", "[Rd+imm], Rs", "addr"};
      for (size_t i = 0; i < n; ++i) std::cout << "0x" << std::right << std::hex << std::setw(2) << std::setfill('0') << int(static_cast<uint8_t>(ops[i].op)) << std::dec << std::setfill(' ') << "    " << std::left << std::setw(10) << ops[i].name << std::setw(16) << forms[static_cast<int>(ops[i].form)] << ops[i].summary << "\n";
      std::cout << "\nEncoding: 32 bits = [opcode:8 | rd:4 | rs:4] [imm:16]. 8 registers, 64K 16-bit words of memory, stack grows down from 0xFF00.\n";
      return 0;
    }
    if (argc < 3) return usage();
    const std::string path = argv[2];
    Program prog = assemble(readFile(path));
    if (cmd == "asm") { std::cout << listing(prog); std::cout << prog.image.size() << " words\n"; return 0; }
    bool trace = false, color = isatty(1); uint64_t max = 1'000'000; std::string html;
    for (int i = 3; i < argc; ++i) {
      std::string a = argv[i];
      if (a == "--trace") trace = true; else if (a == "--no-color") color = false;
      else if (a == "--max" && i + 1 < argc) max = std::stoull(argv[++i]);
      else if (a == "--html" && i + 1 < argc) html = argv[++i];
      else { std::cerr << "unknown option " << a << "\n"; return usage(); }
    }
    Cpu cpu; cpu.load(prog.image);
    if (cmd == "run") {
      std::vector<TraceEntry> t;
      uint64_t steps = cpu.run(max, (trace || !html.empty()) ? &t : nullptr);
      if (trace) {
        for (auto& e : t) std::cout << "0x" << std::right << std::hex << std::setw(4) << std::setfill('0') << e.before.pc << std::dec << std::setfill(' ') << "  " << std::left << std::setw(20) << e.text << "\n" << registerPanel(e.before, e.after, color) << "\n\n";
      }
      std::cout << "Halted after " << steps << " instructions.\nOutput:";
      for (auto v : cpu.output()) std::cout << " " << v;
      std::cout << "\n" << registerPanel(cpu.state(), cpu.state(), false) << "\n";
      if (!html.empty()) { std::ofstream(html) << htmlTrace(prog, t, cpu.output(), path); std::cout << "Trace viewer written to " << html << "\n"; }
      return 0;
    }
    if (cmd == "step") {
      std::string line; bool cont = false;
      while (!cpu.state().halted) {
        Instr next = cpu.fetchAt(cpu.state().pc);
        auto it = prog.sourceLine.find(cpu.state().pc);
        if (!cont) {
          std::cout << "\nnext 0x" << std::hex << cpu.state().pc << std::dec << ": " << disassemble(next);
          if (it != prog.sourceLine.end()) std::cout << "    ; line " << it->second;
          std::cout << "\n> " << std::flush;
          if (!std::getline(std::cin, line) || line == "q") break;
          if (line == "c") cont = true;
          if (line.rfind("m ", 0) == 0) {
            std::istringstream ss(line.substr(2)); std::string a; int n = 8; ss >> a >> n;
            const uint16_t addr = static_cast<uint16_t>(std::stoul(a, nullptr, 0));
            for (int k = 0; k < n; ++k) {
              std::cout << "[0x" << std::hex << (addr + k) << "] = " << std::dec << cpu.mem(static_cast<uint16_t>(addr + k)) << "\n";
            }
            continue;
          }
        }
        TraceEntry t = cpu.step();
        if (!cont) std::cout << registerPanel(t.before, t.after, color) << "\n";
      }
      std::cout << "\nOutput:"; for (auto v : cpu.output()) std::cout << " " << v; std::cout << "\n" << registerPanel(cpu.state(), cpu.state(), false) << "\n";
      return 0;
    }
    return usage();
  } catch (const AsmError& e) { std::cerr << "assembly error: " << e.what() << "\n"; return 1; }
  catch (const CpuError& e) { std::cerr << "CPU fault: " << e.what() << "\n"; return 1; }
  catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
}
