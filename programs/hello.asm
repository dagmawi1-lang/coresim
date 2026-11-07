; Print "HELLO" as character codes (72 69 76 76 79) from a string table.
        LDI R1, msg
next:   LOAD R2, [R1]
        CMPI R2, 0
        JZ done
        OUT R2
        ADDI R1, 1
        JMP next
done:   HALT
msg:    .word 'H', 'E', 'L', 'L', 'O', 0
