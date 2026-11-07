# CoreSim — a CPU instruction simulator in C++

CoreSim simulates **CS16**, a small 16-bit load/store CPU I designed for this project. It includes a two-pass assembler, a cycle-counting simulator with flags and a stack, an interactive stepper, and an HTML trace viewer for watching registers change instruction by instruction.

**Language:** C++17, no external libraries
**Shows:** computer architecture (fetch–decode–execute, ALU flags, addressing modes, calling convention, stack), low-level programming, testing.

## Build and run
```bash
make            # builds ./coresim (or: cmake -B build && cmake --build build)
make test       # 17 tests / 39 checks
./coresim run programs/fibonacci.asm
./coresim run programs/factorial.asm --html factorial_trace.html   # open in a browser: Step / Back / Play
./coresim step programs/sum_array.asm      # Enter = step, c = continue, m 0x14 6 = dump memory, q = quit
./coresim asm programs/bubble_sort.asm     # address · machine code · disassembly · source
./coresim isa                              # instruction set reference
```

## The CS16 architecture
| Part | Design |
|---|---|
| Registers | R0–R7 (16-bit), PC, SP; flags **Z** (zero), **N** (negative), **C** (carry/borrow), **V** (signed overflow) |
| Memory | 65,536 16-bit words, von Neumann (code and data share memory), stack grows down from `0xFF00` |
| Encoding | 32 bits = two words: `[opcode:8 | rd:4 | rs:4] [imm:16]` |
| Addressing | immediate, register, base+offset (`LOAD R1, [R2+3]`), absolute (jumps) |
| Instructions | 29: data movement, arithmetic/logic with flags, shifts, compare, conditional branches (signed `JN`/`JGE`, unsigned `JC`), `CALL`/`RET`, `PUSH`/`POP`, `OUT` |

**Fetch–decode–execute:** `Cpu::step()` reads two words at PC, decodes the opcode and register fields, advances PC by 2, then executes. Flag rules match real CPUs: for subtraction, C means *borrow* and V is set when the operands' signs differ and the result's sign differs from the first operand. Signed comparison (`JN` = "less than") uses N ≠ V, which is the same trick ARM and x86 use.

**Faults are caught, not undefined:** illegal opcodes, stack overflow/underflow, executing after HALT, and runaway loops (step limit) raise `CpuError` with the address.

## Example programs (`programs/`)
| Program | Demonstrates | Output |
|---|---|---|
| `fibonacci.asm` | loops, register moves | 0 1 1 2 3 5 8 13 21 34 |
| `sum_array.asm` | memory, pointer arithmetic, `.word` data | 23 |
| `factorial.asm` | **recursion** with CALL/RET and the stack | 720 |
| `bubble_sort.asm` | nested loops, `.equ`, signed compare, in-place STORE | -3 0 7 7 19 42 100 |
| `hello.asm` | null-terminated string, character literals | 72 69 76 76 79 ("HELLO") |

## Assembler syntax
```asm
        .equ N, 7          ; constant
start:  LDI R1, 'A'        ; char, decimal, 0x hex, 0b binary, label, or -negative
        LOAD R2, [R1+2]    ; base + offset
        STORE [R1-1], R2
data:   .word 1, 2, 3      ; .space 10 reserves zeroed words
```
Errors report the line number, for example `line 2: ADD expects 2 operand(s), got 1`.

## Code layout
`src/isa.*` opcode table and disassembler · `src/cpu.*` simulator · `src/assembler.*` two-pass assembler · `src/view.*` terminal panel and HTML trace · `tests/test_main.cpp` dependency-free test runner. Builds clean with `-Wall -Wextra -Wpedantic -Werror`, and the tests pass under AddressSanitizer and UBSan.

## License

**Permission required.** © 2026 Dagmawi Melaku. You need my written permission before using, copying, or modifying this code, and any approved use must credit me with a link to this repository. Viewing, forking on GitHub, and running it to judge or review a submission are allowed without asking. To ask, [open an issue](../../issues/new). See [LICENSE](LICENSE) for the full terms.
