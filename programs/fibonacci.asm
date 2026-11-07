; Print the first 10 Fibonacci numbers.
        LDI R1, 0          ; a
        LDI R2, 1          ; b
        LDI R3, 10         ; counter
loop:   OUT R1
        MOV R4, R1
        ADD R4, R2         ; next = a + b
        MOV R1, R2
        MOV R2, R4
        ADDI R3, -1
        JNZ loop
        HALT
