# Test results — 2026-11-07
```
PASS encoding round-trip
PASS ADD sets carry and zero
PASS signed overflow flag
PASS SUB borrow
PASS shifts and logic
PASS signed compare branches
PASS memory load/store with offsets
PASS stack push/pop order
PASS program: fibonacci
PASS program: sum array
PASS program: recursive factorial
PASS program: bubble sort (signed)
PASS program: hello
PASS assembler errors carry line numbers
PASS comments, char literals, hex, labels on own line
PASS faults: infinite loop, stack underflow, illegal opcode
PASS trace records memory writes and cycles

17 tests, 39 checks, 0 failures
```
- Same suite under `-fsanitize=address,undefined`: 17 tests, 39 checks, 0 failures
- CMake + CTest build: passes
- Compiler: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, flags `-Wall -Wextra -Wpedantic -Werror`
