; Sum an array stored in memory with LOAD and pointer arithmetic.
        LDI R1, data       ; pointer
        LDI R2, 0          ; sum
        LDI R3, 6          ; length
loop:   LOAD R4, [R1]
        ADD R2, R4
        ADDI R1, 1
        ADDI R3, -1
        JNZ loop
        OUT R2             ; 3+1+4+1+5+9 = 23
        HALT
data:   .word 3, 1, 4, 1, 5, 9
