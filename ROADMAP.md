# CoreSim roadmap

## Done (v1.0.0)
- [x] Simulated registers and memory (8 × 16-bit registers, 64K words, flags, stack)
- [x] Instruction execution (29 instructions, faults for illegal states)
- [x] Step-by-step execution (interactive `step` mode, `--trace`)
- [x] Instruction and register visualization (terminal panel with changed registers highlighted, HTML trace viewer)
- [x] Basic CPU architecture concepts documented in the README
- [x] Assembler with labels, constants, data directives, and line-numbered errors
- [x] 17 tests (39 checks), clean under ASan/UBSan and `-Werror`

## Planned improvements
- [ ] 5-stage pipeline mode (IF/ID/EX/MEM/WB) with hazard detection and stall counts
- [ ] Simple direct-mapped cache with hit/miss statistics
- [ ] Interrupts and a memory-mapped timer
- [ ] Breakpoints in step mode
