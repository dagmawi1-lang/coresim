; Recursive factorial using CALL/RET and the stack. Computes 6! = 720.
        LDI R1, 6
        CALL fact
        OUT R0
        HALT
; fact(R1) -> R0
fact:   CMPI R1, 1
        JN base            ; signed n < 1
        JZ base            ; n == 1
        PUSH R1
        ADDI R1, -1
        CALL fact          ; R0 = fact(n-1)
        POP R1
        MUL R0, R1         ; R0 = n * fact(n-1)
        RET
base:   LDI R0, 1
        RET
