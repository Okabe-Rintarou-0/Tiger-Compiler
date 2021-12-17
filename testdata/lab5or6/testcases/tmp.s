.text
.globl g
.type g, @function
.set g_framesize, 8
g:
subq $8, %rsp
L6:
movq %rbx, t102
leaq g_framesize(%rsp), t108 
movq t108, t103
movq %r12, t104
movq %r13, t105
movq %r14, t106
movq %r15, t107
leaq g_framesize(%rsp), t110 
movq $-8, t111
movq t110, t109
addq t111, t109
movq %rdi, (t109)
movq %rsi, t100
movq $3, t112
cmp t112, t100
jg L0
L1:
movq $4, t113
leaq g_framesize(%rsp), t116 
movq $-8, t117
movq t116, t115
addq t117, t115
movq (t115), t118
movq $-8, t119
movq t118, t114
addq t119, t114
movq t113, (t114)
movq $0, t120
movq t120, t101
L3:
movq t101, %rax
movq t102, %rbx
leaq g_framesize(%rsp), t121 
movq t103, t121
movq t104, %r12
movq t105, %r13
movq t106, %r14
movq t107, %r15
jmp L5
L0:
leaq L2(%rip), t123
movq t123, %rdi
callq print
movq %rax, t122
movq t122, t101
jmp L3
L5:


addq $8, %rsp
retq
.size g, .-g
.globl tigermain
.type tigermain, @function
.set tigermain_framesize, 8
tigermain:
subq $8, %rsp
L8:
movq $5, t124
leaq tigermain_framesize(%rsp), t126 
movq $-8, t127
movq t126, t125
addq t127, t125
movq t124, (t125)
leaq tigermain_framesize(%rsp), t130 
movq $-8, t131
movq t130, t129
addq t131, t129
movq (t129), t132
movq t132, %rdi
callq printi
movq %rax, t128
leaq L4(%rip), t134
movq t134, %rdi
callq print
movq %rax, t133
leaq tigermain_framesize(%rsp), t136 
movq t136, %rdi
movq $2, t137
movq t137, %rsi
callq g
movq %rax, t135
leaq tigermain_framesize(%rsp), t140 
movq $-8, t141
movq t140, t139
addq t141, t139
movq (t139), t142
movq t142, %rdi
callq printi
movq %rax, t138
jmp L7
L7:


addq $8, %rsp
retq
.size tigermain, .-tigermain
.section .rodata
L2:
.long 20
.string "hey! Bigger than 3!\n"
L4:
.long 1
.string "\n"
