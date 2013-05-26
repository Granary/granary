Summary of instruction forms found in Linux 3.8.2
=================================================
### POPF
```asm
popfq
```

### ENTER
```asm
enterq $imm, $imm
```

### JS
```asm
js (%ip64)
```

### JL
```asm
jl (%ip64)
```

### MOVZ
```asm
movzwq (%reg64,%reg64,1), %reg64
movzbq (%reg64), %reg64
movzbq (%reg64,%reg64,1), %reg64
movzbl %reg8, %reg32
movzwl %reg16, %reg32
movzbl (%reg64), %reg32
movzbl (%ip64), %reg32
movzbl (%reg64,%reg64,1), %reg32
movzwl (%reg64), %reg32
movzwl (%ip64), %reg32
movzwl (%reg64,%reg64,1), %reg32
movzwl (%mmap), %reg32
```

### JE
```asm
je (%ip64)
```

### JG
```asm
jg (%ip64)
```

### JA
```asm
ja (%ip64)
```

### JB
```asm
jb (%ip64)
```

### SAHF
```asm
sahf
```

### LRET
```asm
lretq
```

### CLFLUSH
```asm
clflush (%reg64)
```

### WRMSR
```asm
wrmsr
```

### FWAIT
```asm
fwait
```

### SAR
```asm
sarq %reg8, (%reg64)
sarl %reg8, (%reg64)
sarl %reg32
sarb %reg8
sarq %reg64
sarq $imm, %reg64
sarq %reg8, %reg64
sarw $imm, %reg16
sarl $imm, %reg32
sarb $imm, %reg8
sarq $imm, (%reg64)
sarl %reg8, %reg32
```

### CQTO
```asm
cqto
```

### PCLMULLQHQD
```asm
pclmullqhqdq %simd128, %simd128
```

### LJMP
```asm
ljmpq (%reg64)
```

### MOVNTI
```asm
movntiq %reg64, (%reg64)
```

### CMOVS
```asm
cmovsl %reg32, %reg32
cmovsq (%reg64), %reg64
cmovsq %reg64, %reg64
cmovsl (%reg64), %reg32
```

### FXRSTOR64
```asm
fxrstor64 (%reg64)
```

### CMOVL
```asm
cmovll %reg32, %reg32
cmovlq (%reg64), %reg64
cmovlq %reg64, %reg64
cmovll (%reg64), %reg32
```

### CMOVAE
```asm
cmovael %reg32, %reg32
cmovaeq (%reg64), %reg64
cmovaeq (%ip64), %reg64
cmovaew (%reg64), %reg16
cmovaeq %reg64, %reg64
cmovael (%reg64), %reg32
cmovael (%ip64), %reg32
```

### CMOVA
```asm
cmoval %reg32, %reg32
cmovaq (%reg64), %reg64
cmovaq %reg64, %reg64
cmoval (%reg64), %reg32
```

### CMOVB
```asm
cmovbl %reg32, %reg32
cmovbq (%reg64), %reg64
cmovbq %reg64, %reg64
cmovbl (%reg64), %reg32
cmovbl (%ip64), %reg32
```

### JAE
```asm
jae (%ip64)
```

### CMOVE
```asm
cmovel %reg32, %reg32
cmoveq (%reg64), %reg64
cmoveq (%ip64), %reg64
cmovew (%reg64), %reg16
cmoveq %reg64, %reg64
cmovel (%reg64), %reg32
cmovel (%ip64), %reg32
cmovel (%reg64,%reg64,1), %reg32
```

### CMOVG
```asm
cmovgl %reg32, %reg32
cmovgq %reg64, %reg64
cmovgl (%reg64), %reg32
```

### JNS
```asm
jns (%ip64)
```

### BT
```asm
btl $imm, (%reg64)
btl %reg32, %reg32
btq %reg64, %reg64
btl $imm, %reg32
btl %reg32, (%reg64)
btl %reg32, (%ip64)
btl %reg32, (%reg64,%reg64,1)
```

### IDIV
```asm
idivq %reg64
idivq (%reg64)
idivl (%reg64)
idivl (%ip64)
idivl %reg32
```

### FXSAVE
```asm
fxsave (%ip64)
```

### JNE
```asm
jne (%ip64)
```

### SYSEXIT
```asm
sysexit
```

### OR
```asm
orb %reg8, (%reg64)
orb %reg8, (%reg64,%reg64,1)
orw %reg16, %reg16
orw %reg16, (%reg64)
orw %reg16, (%reg64,%reg64,1)
orl $imm, %reg32
orb $imm, %reg8
repnz orl $imm, %reg32
lock orb $imm, (%reg64)
lock orb $imm, (%ip64)
orl %reg32, (%reg64)
orl %reg32, (%ip64)
orl %reg32, (%reg64,%reg64,1)
orb (%reg64), %reg8
orb (%reg64,%reg64,1), %reg8
orb %reg8, %reg8
orl.s %reg32, %reg32
orb $imm, (%reg64)
orb $imm, (%ip64)
orb $imm, (%reg64,%reg64,1)
orq %reg64, (%reg64)
orq %reg64, (%ip64)
orq %reg64, (%reg64,%reg64,1)
orq %reg64, %reg64
orq (%reg64), %reg64
orq (%ip64), %reg64
orq (%reg64,%reg64,1), %reg64
orl $imm, (%reg64)
orl $imm, (%ip64)
orl $imm, (%reg64,%reg64,1)
orw $imm, (%reg64)
orq $imm, %reg64
orw $imm, %reg16
orq $imm, (%reg64)
orq $imm, (%ip64)
orq $imm, (%reg64,%reg64,1)
orl %reg32, %reg32
rex orl (%reg64), %reg32
orw (%reg64), %reg16
orw (%reg64,%reg64,1), %reg16
orl (%reg64), %reg32
orl (%ip64), %reg32
orl (%reg64,%reg64,1), %reg32
```

### CMPS
```asm
repz cmpsb %seg64:(%reg64), %seg64:(%reg64)
```

### LEAVE
```asm
leaveq
```

### XOR
```asm
xorl $imm, (%reg64)
xorl $imm, (%ip64)
xorb %reg8, (%reg64)
xorw %reg16, %reg16
xorw $imm, (%reg64)
xorl %reg32, (%reg64)
xorl %reg32, (%ip64)
xorq %reg64, (%reg64)
xorb (%reg64), %reg8
xorb (%reg64,%reg64,1), %reg8
xorq $imm, %reg64
xorq %reg64, %reg64
xorw $imm, %reg16
xorl $imm, %reg32
xorb %reg8, %reg8
xorb $imm, %reg8
xorq (%reg64), %reg64
xorq (%reg64,%reg64,1), %reg64
xorw %reg16, (%reg64)
xorl %reg32, %reg32
xorw (%reg64), %reg16
xorw (%reg64,%reg64,1), %reg16
xorb $imm, (%reg64)
xorb $imm, (%reg64,%reg64,1)
xorl (%reg64), %reg32
xorl (%ip64), %reg32
xorl (%reg64,%reg64,1), %reg32
```

### DEC
```asm
decl %reg32
decb %reg8
decq %reg64
lock decq (%reg64)
lock decq (%ip64)
lock decw (%reg64)
decb (%reg64)
decq (%reg64)
decq %seg64:(%reg64)
decl (%reg64)
decl (%ip64)
decl %seg64:(%reg64)
decw (%reg64)
lock decl (%reg64)
lock decl (%ip64)
lock decl (%reg64,%reg64,1)
```

### XADD
```asm
xaddb %reg8, %seg64:(%reg64)
lock xaddq %reg64, (%reg64)
lock xaddq %reg64, (%ip64)
lock xaddl %reg32, (%reg64)
lock xaddl %reg32, (%ip64)
lock xaddl %reg32, (%reg64,%reg64,1)
xaddq %reg64, (%reg64)
```

### CMOVBE
```asm
cmovbel %reg32, %reg32
cmovbeq (%reg64), %reg64
cmovbeq (%reg64,%reg64,1), %reg64
cmovbeq %reg64, %reg64
cmovbel (%reg64), %reg32
cmovbel (%ip64), %reg32
```

### CMP
```asm
cmpl $imm, (%reg64)
cmpl $imm, (%ip64)
cmpl $imm, (%reg64,%reg64,1)
cmpb %reg8, (%reg64)
cmpb %reg8, (%ip64)
cmpb %reg8, (%reg64,%reg64,1)
cmpq %reg64, (%reg64)
cmpq %reg64, (%ip64)
cmpq %reg64, (%reg64,%reg64,1)
cmpw $imm, (%reg64)
cmpw $imm, (%ip64)
cmpw $imm, (%reg64,%reg64,1)
cmpl %reg32, (%reg64)
cmpl %reg32, (%ip64)
cmpl %reg32, (%reg64,%reg64,1)
cmpw %reg16, (%reg64)
cmpw %reg16, (%ip64)
cmpw %reg16, (%reg64,%reg64,1)
cmpw %reg16, %reg16
cmpq $imm, %reg64
cmpq %reg64, %reg64
cmpw $imm, %reg16
cmpl $imm, %reg32
cmpb %reg8, %reg8
cmpb $imm, %reg8
cmpl $imm, mem
cmpq (%reg64), %reg64
cmpq (%ip64), %reg64
cmpq %seg64:(%reg64), %reg64
cmpq (%reg64,%reg64,1), %reg64
cmpq $imm, (%reg64)
cmpq $imm, (%ip64)
cmpq $imm, (%reg64,%reg64,1)
cmpb (%reg64), %reg8
cmpb (%ip64), %reg8
cmpb (%reg64,%reg64,1), %reg8
cmpl %reg32, %reg32
cmpw (%reg64), %reg16
cmpw (%ip64), %reg16
cmpw (%reg64,%reg64,1), %reg16
cmpb $imm, (%reg64)
cmpb $imm, (%ip64)
cmpb $imm, (%reg64,%reg64,1)
cmpl (%reg64), %reg32
cmpl (%ip64), %reg32
cmpl (%reg64,%reg64,1), %reg32
```

### SBB
```asm
sbbl $imm, (%reg64)
sbbb %reg8, (%reg64)
sbbq $imm, %reg64
sbbq %reg64, %reg64
sbbw $imm, %reg16
sbbl $imm, %reg32
sbbb $imm, %reg8
sbbl %reg32, %reg32
```

### LOOPNE
```asm
loopneq (%ip64)
```

### ROL
```asm
roll %reg32
rolq %reg8, %reg64
rolw $imm, %reg16
roll $imm, %reg32
rolb $imm, %reg8
roll %reg8, %reg32
```

### JMP
```asm
jmp (%ip64)
jmpq %reg64
jmpq (%ip64)
```

### MOVS
```asm
movslq (%reg64), %reg64
movslq (%ip64), %reg64
movslq (%reg64,%reg64,1), %reg64
movswq (%reg64), %reg64
movsbq (%reg64), %reg64
movsbq (%reg64,%reg64,1), %reg64
movsbl %reg8, %reg32
movsbq %reg8, %reg64
movswq %reg16, %reg64
movswl (%reg64), %reg32
movswl (%ip64), %reg32
movswl (%reg64,%reg64,1), %reg32
movswl %reg16, %reg32
rep movsl %seg64:(%reg64), %seg64:(%reg64)
movsbw %reg8, %reg16
rep movsb %seg64:(%reg64), %seg64:(%reg64)
movsl %seg64:(%reg64), %seg64:(%reg64)
movsw %seg64:(%reg64), %seg64:(%reg64)
rep movsq %seg64:(%reg64), %seg64:(%reg64)
movslq %reg32, %reg64
movsbl (%reg64), %reg32
movsbl (%ip64), %reg32
movsbl (%reg64,%reg64,1), %reg32
movsb %seg64:(%reg64), %seg64:(%reg64)
movsbw (%reg64), %reg16
movsbw (%reg64,%reg64,1), %reg16
```

### PXOR
```asm
pxor %simd128, %simd128
```

### MOVAP
```asm
movaps %simd128, (%reg64)
movaps (%reg64), %simd128
```

### INVLPG
```asm
invlpg (%reg64)
```

### XCHG
```asm
xchgb %reg8, (%reg64)
data32 xchgw %reg16, %reg16
xchgl %reg32, (%reg64)
xchgl %reg32, (%ip64)
xchgq %reg64, %reg64
xchgq %reg64, (%reg64)
xchgq %reg64, (%ip64)
xchgq %reg64, (%reg64,%reg64,1)
xchgw %reg16, %reg16
xchgl %reg32, %reg32
```

### ROR
```asm
rorb %reg8
rorl $imm, %reg32
```

### LLD
```asm
lldt %reg16
```

### SHR
```asm
shrl $imm, (%reg64)
shrq %reg8, (%reg64)
shrq %reg8, (%ip64)
shrw %reg16
shrl %reg8, (%reg64)
shrl %reg32
shrb %reg8
shrq %reg64
shrq $imm, %reg64
shrq %reg8, %reg64
shrw $imm, %reg16
shrl $imm, %reg32
shrb $imm, %reg8
shrb (%reg64)
shrq (%reg64)
shrq $imm, (%reg64)
shrl %reg8, %reg32
shrw (%reg64)
shrdq $imm, %reg64, %reg64
shrb $imm, (%reg64)
```

### SETLE
```asm
setle %reg8
setle (%reg64)
```

### MONITOR
```asm
monitor %reg64, %reg64, %reg64
```

### SFENCE
```asm
sfence
```

### XSAVE64
```asm
xsave64 (%reg64)
```

### JGE
```asm
jge (%ip64)
```

### STI
```asm
sti
```

### RDRAND
```asm
rdrand %reg64
```

### PCLMULLQLQD
```asm
pclmullqlqdq %simd128, %simd128
```

### SHL
```asm
shll $imm, (%reg64)
shlq %reg8, (%reg64)
shlq %reg8, (%ip64)
shll %reg8, (%reg64)
shll %reg32
shlq %reg64
shlq $imm, %reg64
shlq %reg8, %reg64
shll $imm, %reg32
shlb $imm, %reg8
shlb (%reg64)
shlq (%reg64)
shlq $imm, (%reg64)
shll %reg8, %reg32
shlw (%reg64)
shll (%reg64)
```

### XRSTOR64
```asm
xrstor64 (%reg64)
```

### AND
```asm
andl $imm, (%reg64)
andl $imm, (%ip64)
andl $imm, (%reg64,%reg64,1)
andb %reg8, (%reg64)
andb %reg8, (%ip64)
andb %reg8, (%reg64,%reg64,1)
andq %reg64, (%reg64)
andq %reg64, (%ip64)
andq %reg64, (%reg64,%reg64,1)
andw $imm, (%reg64)
andw $imm, (%reg64,%reg64,1)
lock andb $imm, (%reg64)
lock andb $imm, (%ip64)
andl %reg32, (%reg64)
andl %reg32, (%ip64)
andl %reg32, (%reg64,%reg64,1)
andb (%reg64), %reg8
andb (%ip64), %reg8
andb (%reg64,%reg64,1), %reg8
andw %reg16, %reg16
andq $imm, %reg64
andq %reg64, %reg64
andw $imm, %reg16
andl $imm, %reg32
andb %reg8, %reg8
andb $imm, %reg8
andq (%reg64), %reg64
andq (%ip64), %reg64
andq (%reg64,%reg64,1), %reg64
andq $imm, (%reg64)
andq $imm, (%ip64)
andq $imm, (%reg64,%reg64,1)
andw %reg16, (%reg64)
andw %reg16, (%reg64,%reg64,1)
andl %reg32, %reg32
andw (%reg64), %reg16
andw (%reg64,%reg64,1), %reg16
andb $imm, (%reg64)
andb $imm, (%ip64)
andb $imm, (%reg64,%reg64,1)
andl (%reg64), %reg32
andl (%ip64), %reg32
andl (%reg64,%reg64,1), %reg32
```

### RDTSCP
```asm
rdtscp
```

### OUTS
```asm
rep outsb %seg64:(%reg64), (%reg16)
rep outsw %seg64:(%reg64), (%reg16)
rep outsl %seg64:(%reg64), (%reg16)
outsb %seg64:(%reg64), (%reg16)
```

### MFENCE
```asm
mfence
```

### BTR
```asm
lock btrl $imm, (%reg64)
lock btrl $imm, (%ip64)
btrl $imm, (%reg64)
btrl %reg32, (%reg64)
btrl %reg32, (%ip64)
lock btrl %reg32, (%reg64)
lock btrl %reg32, (%ip64)
```

### SGDT
```asm
sgdt (%reg64)
```

### HLT
```asm
hlt
```

### MOV
```asm
movb %reg8, %seg64:(%reg64)
movb %reg8, (%reg64)
movb %reg8, (%ip64)
movb %reg8, (%reg32)
movb %reg8, (%mmap)
movb %reg8, (%reg64,%reg64,1)
movw %seg64, %reg16
movw %seg64, (%reg64)
movw %seg64, (%ip64)
movw %seg64, (%reg64,%reg64,1)
movw %seg64, (%mmap)
movw %seg64, (%reg32)
movw %reg16, (%reg64)
movw %reg16, (%ip64)
movw %reg16, (%reg64,%reg64,1)
movw %reg16, (%mmap)
movw %reg16, (%reg32)
movl $imm, %reg32
movb $imm, %reg8
movl %reg32, %seg64:(%reg64)
movl %reg32, (%reg64)
movl %reg32, (%ip64)
movl %reg32, (%reg32)
movl %reg32, (%mmap)
movl %reg32, (%reg64,%reg64,1)
movb (%reg64), %reg8
movb (%ip64), %reg8
movb %seg64:(%reg64), %reg8
movb (%reg64,%reg64,1), %reg8
movb (%reg32), %reg8
movb %reg8, %reg8
movb $imm, (%reg64)
movb $imm, (%ip64)
movb $imm, (%reg64,%reg64,1)
movb $imm, (%mmap)
movq %reg64, mem
movq %reg64, (%reg64)
movq %reg64, (%ip64)
movq %reg64, %seg64:(%reg64)
movq %reg64, (%reg64,%reg64,1)
movq %reg64, (%mmap)
movq %control64, %control64
movq %control64, %debug64
movq %control64, %reg64
movq %control64, %simd128
movq %debug64, %control64
movq %debug64, %debug64
movq %debug64, %reg64
movq %debug64, %simd128
movq %reg64, %control64
movq %reg64, %debug64
movq %reg64, %reg64
movq %reg64, %simd128
movq %simd128, %control64
movq %simd128, %debug64
movq %simd128, %reg64
movq %simd128, %simd128
movl %reg32, mem
movq mem, %reg64
movq (%reg64), %reg64
movq (%ip64), %reg64
movq %seg64:(%reg64), %reg64
movq (%reg64,%reg64,1), %reg64
movq (%mmap), %reg64
movl $imm, (%reg64)
movl $imm, (%ip64)
movl $imm, %seg64:(%reg64)
movl $imm, (%reg64,%reg64,1)
movl $imm, (%mmap)
movw $imm, (%reg64)
movw $imm, (%ip64)
movw $imm, (%reg64,%reg64,1)
movq $imm, %reg64
movw $imm, %reg16
movl mem, %reg32
movq $imm, (%reg64)
movq $imm, (%ip64)
movq $imm, %seg64:(%reg64)
movq $imm, (%reg64,%reg64,1)
movl %seg64, %seg64
movl %seg64, %reg32
movl %reg32, %seg64
movl %reg32, %reg32
movw (%reg64), %reg16
movw (%ip64), %reg16
movw (%reg64,%reg64,1), %reg16
movw (%reg32), %reg16
movl %seg64:(%reg64), %reg32
movl (%reg64), %reg32
movl (%ip64), %reg32
movl (%reg32), %reg32
movl (%mmap), %reg32
movl (%reg64,%reg64,1), %reg32
```

### INS
```asm
rep insb (%reg16), %seg64:(%reg64)
rep insw (%reg16), %seg64:(%reg64)
rep insl (%reg16), %seg64:(%reg64)
```

### JLE
```asm
jle (%ip64)
```

### MOVDQA
```asm
movdqa (%reg64), %simd128
```

### PUSHF
```asm
pushfq
```

### NOT
```asm
notl (%reg64)
notq %reg64
notq (%reg64)
notw (%reg64)
notw (%ip64)
notl %reg32
```

### MOVABS
```asm
movabsb mem, %reg8
movabsq $imm, %reg64
movabsw %reg16, mem
```

### SYSRET
```asm
sysretq
sysretl
```

### INC
```asm
incw %reg16
incl %reg32
incq %reg64
lock incq (%reg64)
lock incq (%ip64)
lock incq (%reg64,%reg64,1)
lock incw (%reg64)
incb (%reg64)
incq (%reg64)
incq %seg64:(%reg64,%reg64,1)
incq (%ip64)
incq %seg64:(%reg64)
incl (%reg64)
incl (%ip64)
incl %seg64:(%reg64)
incl (%reg64,%reg64,1)
incw (%reg64)
lock incl (%reg64)
lock incl (%ip64)
lock incl (%reg64,%reg64,1)
```

### SUB
```asm
subb %reg8, (%reg64)
subw %reg16, (%reg64)
subw %reg16, (%reg64,%reg64,1)
subl $imm, %reg32
subb $imm, %reg8
lock subq %reg64, (%reg64)
lock subq %reg64, (%ip64)
subl %reg32, (%reg64)
subl %reg32, (%ip64)
subl %reg32, (%reg64,%reg64,1)
lock subl $imm, (%reg64)
lock subl $imm, (%ip64)
subb (%reg64), %reg8
subb $imm, (%reg64)
subb $imm, (%reg64,%reg64,1)
subq %reg64, (%reg64)
subq %reg64, (%ip64)
subq %reg64, %reg64
lock subq $imm, (%ip64)
subq (%reg64), %reg64
subq (%ip64), %reg64
subq (%reg64,%reg64,1), %reg64
subl $imm, (%reg64)
subl $imm, (%ip64)
subl $imm, (%reg64,%reg64,1)
subw $imm, (%reg64)
subw $imm, (%reg64,%reg64,1)
subq $imm, %reg64
subw $imm, %reg16
lock subl %reg32, (%reg64)
lock subl %reg32, (%ip64)
lock subl %reg32, (%reg64,%reg64,1)
subq $imm, (%reg64)
subq $imm, (%ip64)
subq $imm, (%reg64,%reg64,1)
subl %reg32, %reg32
subw (%reg64), %reg16
subw (%reg64,%reg64,1), %reg16
subl (%reg64), %reg32
subl (%ip64), %reg32
subl (%reg64,%reg64,1), %reg32
```

### NEG
```asm
negq %reg64
negq (%reg64)
negl (%reg64)
negl %reg32
```

### RET
```asm
retq
repz retq
retq $imm
```

### STOS
```asm
rep stosl %reg32, %seg64:(%reg64)
stosb %reg8, %seg64:(%reg64)
stosq %reg64, %seg64:(%reg64)
stosl %reg32, %seg64:(%reg64)
stosw %reg16, %seg64:(%reg64)
rep stosq %reg64, %seg64:(%reg64)
rep stosb %reg8, %seg64:(%reg64)
```

### JBE
```asm
jbe (%ip64)
```

### LIDT
```asm
lidt (%reg64)
```

### MUL
```asm
mulb %reg8
mulq %reg64
mulb (%reg64)
mulq (%reg64)
mull %reg32
```

### FXSAVE64
```asm
fxsave64 (%reg64)
```

### WBINV
```asm
wbinvd
```

### SETE
```asm
sete %reg8
sete (%reg64)
sete (%ip64)
```

### SETG
```asm
setg %reg8
setg (%reg64)
```

### SETA
```asm
seta %reg8
seta (%reg64)
```

### SETB
```asm
setb %reg8
```

### SETL
```asm
setl %reg8
```

### CWT
```asm
cwtl
```

### BSR
```asm
bsrl %reg32, %reg32
bsrq (%reg64), %reg64
bsrq (%reg64,%reg64,1), %reg64
bsrq %reg64, %reg64
bsrl (%reg64), %reg32
bsrl (%ip64), %reg32
```

### SETS
```asm
sets %reg8
```

### PUSH
```asm
pushq %reg64
pushq (%reg64)
pushq $imm
```

### RDTSC
```asm
rdtsc
```

### BSF
```asm
bsfl %reg32, %reg32
bsfl (%reg64), %reg32
```

### XSETBV
```asm
xsetbv
```

### OUT
```asm
outb %reg8, (%reg16)
outl %reg32, $imm
outl %reg32, (%reg16)
outw %reg16, (%reg16)
outb %reg8, $imm
outw %reg16, $imm
```

### LTR
```asm
ltr %reg16
```

### CLI
```asm
cli
```

### TZCNT
```asm
tzcntq (%reg64), %reg64
tzcntq %reg64, %reg64
```

### CLD
```asm
cld
```

### SETNE
```asm
setne %reg8
setne (%reg64)
setne (%ip64)
setne (%reg64,%reg64,1)
```

### CLC
```asm
clc
```

### ADD
```asm
rex.X rex.B rex.R addb %reg8, (%reg64)
addw %reg16, %reg16
addb %reg8, (%reg64)
addb %reg8, (%reg64,%reg64,1)
addw %reg16, (%reg64)
addw %reg16, (%reg64,%reg64,1)
addl $imm, %reg32
addb $imm, %reg8
lock addq %reg64, (%reg64)
lock addq %reg64, (%ip64)
lock addq %reg64, (%reg64,%reg64,1)
addl %reg32, (%reg64)
addl %reg32, %seg64:(%reg64,%reg64,1)
addl %reg32, (%ip64)
addl %reg32, %seg64:(%reg64)
addl %reg32, (%reg64,%reg64,1)
lock addl $imm, (%reg64)
lock addl $imm, (%ip64)
addb (%reg64), %reg8
addb (%reg64,%reg64,1), %reg8
addb %reg8, %reg8
addl.s %reg32, %reg32
addb $imm, (%reg64)
addq %reg64, (%reg64)
addq %reg64, (%ip64)
addq %reg64, %seg64:(%reg64)
addq %reg64, (%reg64,%reg64,1)
addq %reg64, %reg64
lock addq $imm, (%reg64)
lock addq $imm, (%ip64)
addq (%reg64), %reg64
addq (%ip64), %reg64
addq (%reg64,%reg64,1), %reg64
addl $imm, (%reg64)
addl $imm, (%ip64)
addl $imm, (%reg64,%reg64,1)
addw $imm, (%reg64)
addw $imm, (%reg64,%reg64,1)
addq $imm, %reg64
addw $imm, %reg16
lock addl %reg32, (%reg64)
lock addl %reg32, (%ip64)
lock addl %reg32, (%reg64,%reg64,1)
addq $imm, (%reg64)
addq $imm, (%ip64)
addq $imm, %seg64:(%reg64)
addq $imm, (%reg64,%reg64,1)
addl %reg32, %reg32
addb.s %reg8, %reg8
addw (%reg64), %reg16
addw (%ip64), %reg16
addw (%reg64,%reg64,1), %reg16
addl (%reg64), %reg32
addl (%ip64), %reg32
addl (%reg64,%reg64,1), %reg32
```

### ADC
```asm
adcl $imm, (%reg64)
adcq $imm, %reg64
adcq %reg64, %reg64
adcw $imm, %reg16
adcl $imm, %reg32
adcb $imm, %reg8
adcq (%reg64), %reg64
adcl %reg32, %reg32
adcl (%reg64), %reg32
adcl (%reg64,%reg64,1), %reg32
```

### LGDT
```asm
lgdt (%reg64)
lgdt (%ip64)
```

### CLT
```asm
clts
cltq
cltd
```

### RDPMC
```asm
rdpmc
```

### FIADD
```asm
fiaddl (%reg64)
```

### LEA
```asm
leaq mem, %reg64
leaq (%reg64), %reg64
leaq (%ip64), %reg64
leaq (%reg64,%reg64,1), %reg64
leal (%reg64), %reg32
leal (%reg64,%reg64,1), %reg32
```

### VMXOFF
```asm
vmxoff
```

### BTS
```asm
lock btsl $imm, (%reg64)
lock btsl $imm, (%ip64)
btsl $imm, (%reg64)
btsl %reg32, (%reg64)
btsl %reg32, (%ip64)
btsl $imm, %reg32
lock btsl %reg32, (%reg64)
lock btsl %reg32, (%ip64)
```

### SIDT
```asm
sidt (%reg64)
```

### CMOVLE
```asm
cmovlel %reg32, %reg32
cmovleq (%reg64), %reg64
cmovlew (%reg64), %reg16
cmovleq %reg64, %reg64
cmovlel (%reg64), %reg32
cmovlel (%reg64,%reg64,1), %reg32
```

### SETGE
```asm
setge %reg8
```

### SWAPGS
```asm
swapgs
```

### CRC32
```asm
crc32b (%reg64), %reg32
crc32q %reg64, %reg64
crc32b %reg8, %reg32
crc32q (%reg64), %reg64
crc32w (%reg64), %reg32
crc32l (%reg64), %reg32
```

### UD2
```asm
ud2
```

### IMUL
```asm
imull $imm, %reg32, %reg32
imulq $imm, (%reg64), %reg64
imulq $imm, (%ip64), %reg64
imull %reg32
imulq %reg64
imulq %reg64, %reg64
imull $imm, (%reg64), %reg32
imull $imm, (%ip64), %reg32
imull $imm, (%reg64,%reg64,1), %reg32
imulq (%reg64), %reg64
imulq (%ip64), %reg64
imulq (%reg64,%reg64,1), %reg64
imulq $imm, %reg64, %reg64
imulw $imm, %reg16, %reg16
imull %reg32, %reg32
imull (%reg64), %reg32
imull (%ip64), %reg32
imull (%reg64,%reg64,1), %reg32
imull (%mmap), %reg32
```

### PREFETCHT0
```asm
prefetcht0 (%reg64)
prefetcht0 (%reg64,%reg64,1)
```

### CPUI
```asm
cpuid
```

### DIV
```asm
divw %reg16
divl %reg32
divb %reg8
divq %reg64
divq (%reg64)
divl (%reg64)
divl (%ip64)
```

### CMPXCHG16
```asm
lock cmpxchg16b (%reg64)
```

### SCAS
```asm
scasb %seg64:(%reg64), %reg8
repnz scasb %seg64:(%reg64), %reg8
```

### SETBE
```asm
setbe %reg8
```

### POP
```asm
popq %reg64
popq (%reg64)
```

### INT3
```asm
int3
```

### CMPXCHG
```asm
cmpxchgb %reg8, %seg64:(%reg64)
cmpxchgq %reg64, (%reg64)
cmpxchgq %reg64, %seg64:(%reg64)
lock cmpxchgw %reg16, (%reg64)
lock cmpxchgl %reg32, (%reg64)
lock cmpxchgl %reg32, (%ip64)
lock cmpxchgq %reg64, (%reg64)
lock cmpxchgq %reg64, (%ip64)
```

### IRET
```asm
iretq
```

### BSWAP
```asm
bswap %reg32
bswap %reg64
```

### STD
```asm
std
```

### PAUSE
```asm
pause
```

### FNCLEX
```asm
fnclex
```

### BTC
```asm
btcl %reg32, (%reg64)
btcl %reg32, (%ip64)
```

### CMOVGE
```asm
cmovgel %reg32, %reg32
cmovgeq (%ip64), %reg64
cmovgeq %reg64, %reg64
cmovgel (%reg64), %reg32
cmovgel (%reg64,%reg64,1), %reg32
```

### STR
```asm
strq %reg64
```

### IN
```asm
inb $imm, %reg8
inb (%reg16), %reg8
inw (%reg16), %reg16
inw $imm, %reg16
inl (%reg16), %reg32
```

### LFENCE
```asm
lfence
```

### SETAE
```asm
setae %reg8
setae (%reg64)
```

### RDMSR
```asm
rdmsr
```

### NOP
```asm
data32 nopw %seg64:(%reg64,%reg64,1)
nopw %seg64:(%reg64,%reg64,1)
nopw (%reg64,%reg64,1)
nop
nopl (%reg64)
nopl (%reg64,%reg64,1)
```

### CMOVNS
```asm
cmovnsl %reg32, %reg32
cmovnsq (%reg64), %reg64
cmovnsq %reg64, %reg64
cmovnsl (%reg64), %reg32
```

### CALL
```asm
callq mem
callq %reg64
callq (%reg64)
callq (%ip64)
callq (%reg64,%reg64,1)
```

### TEST
```asm
testl $imm, (%reg64)
testl $imm, (%reg64,%reg64,1)
testb %reg8, (%reg64)
testw %reg16, %reg16
testw $imm, (%reg64)
testw $imm, (%reg64,%reg64,1)
testl %reg32, (%reg64)
testl %reg32, (%ip64)
testl %reg32, (%reg64,%reg64,1)
testq %reg64, (%reg64)
testq %reg64, (%ip64)
testq %reg64, (%reg64,%reg64,1)
testw %reg16, (%reg64)
testq $imm, %reg64
testq %reg64, %reg64
testw $imm, %reg16
testl $imm, %reg32
testb %reg8, %reg8
testb $imm, %reg8
testq $imm, (%reg64)
testq $imm, (%reg64,%reg64,1)
testl %reg32, %reg32
testb $imm, (%reg64)
testb $imm, (%ip64)
testb $imm, (%reg64,%reg64,1)
```

### CMOVNE
```asm
cmovnel %reg32, %reg32
cmovneq (%reg64), %reg64
cmovneq (%ip64), %reg64
cmovnew (%reg64), %reg16
cmovneq %reg64, %reg64
cmovnel (%reg64), %reg32
cmovnel (%ip64), %reg32
```

### MWAIT
```asm
mwait %reg64, %reg64
```