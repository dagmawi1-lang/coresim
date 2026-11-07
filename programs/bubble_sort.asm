; Bubble sort an array in place, then print it.
        .equ N, 7
        LDI R5, N
outer:  ADDI R5, -1
        JZ print
        LDI R1, arr
        MOV R6, R5          ; inner counter
inner:  LOAD R2, [R1]
        LOAD R3, [R1+1]
        CMP R3, R2
        JGE noswap          ; already in order (signed)
        STORE [R1], R3
        STORE [R1+1], R2
noswap: ADDI R1, 1
        ADDI R6, -1
        JNZ inner
        JMP outer
print:  LDI R1, arr
        LDI R6, N
ploop:  LOAD R2, [R1]
        OUT R2
        ADDI R1, 1
        ADDI R6, -1
        JNZ ploop
        HALT
arr:    .word 42, 7, -3, 19, 0, 7, 100
