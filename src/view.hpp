#pragma once
#include <string>
#include <vector>
#include "assembler.hpp"
#include "cpu.hpp"

namespace coresim {
std::string registerPanel(const State& before, const State& after, bool color);
std::string listing(const Program& p);
std::string htmlTrace(const Program& p, const std::vector<TraceEntry>& trace, const std::vector<uint16_t>& output, const std::string& title);
}
