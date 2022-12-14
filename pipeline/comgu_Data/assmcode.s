    .data
w1: .word 0x10000000
    .word 0x02000000
    .word 0x10000000
    .text
main:
    lw $2, 0($0)
    and $4, $2, $5
    or $8, $2, $6
    add $9, $4, $2
    slt $1, $6, $7