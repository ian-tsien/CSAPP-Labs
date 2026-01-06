# Solution Process

Due to issues with the version of `Tcl/Tk` used by the lab, I decided to use the TTY form of the interface. So I comment two lines out in file `sim/Makefile`, `TKLIBS` and `TKINC`.

When you encounter `csapp make: flex: No such file or directory` while making in the `sim` directory, you can use command, such as `sudo apt install flex bison`, to solve this problem.

The `arch-lab` was written assuming:

1. GCC <= 5 behavior
2. `-fcommon` default enabled

While the modern GCC (>= 10):

1. Default is `-fno-common`
2. Multiple global definitions are hard errors

We need to manually add this compilation option to `CFLAGS` to maintain compatibility with the behavior of older GCC versions.
We use the regular expression `([L]?CFLAGS[\s]*=.*)` to match options and replace them with `${1} -fcommon`. Then, you can change to the `sim` directory and build the Y86-64 tools.

## Part A

The three Y86-64 programs are quite simple, so I list code with comments, and these should suffice to understand them.

This is `sum.ys`:

```asm
# Execution begins at address 0
.pos 0
irmovq stack, %rsp # Set up stack pointer
call main # Execute main program
halt # Terminate program

# Test linked list
.align 8
ele1:
  .quad 0x00a
  .quad ele2
ele2:
  .quad 0x0b0
  .quad ele3
ele3:
  .quad 0xc00
  .quad 0

main:
   irmovq ele1, %rdi
   call sum_list # sum_list(ele1)
   ret

# long sum_list(list_ptr ls)
# ls in %rdi
sum_list:
  xorq %rax, %rax # val = 0
  andq %rdi, %rdi # Set CC
  jmp test # Goto test
loop:
  mrmovq (%rdi), %r8 # Get ls->val
  addq %r8, %rax # Add to val
  mrmovq 0x8(%rdi), %rdi # Get ls->next
  andq %rdi, %rdi # Set CC
test:
  jne loop # Stop when 0
  ret # Return

# Stack starts here and grows to lower address
.pos 0x200
stack:
```

This is the result after `./yas sum.ys` and `./yis sum.yo`:

```txt
Stopped in 26 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rsp:   0x0000000000000000      0x0000000000000200
%r8:    0x0000000000000000      0x0000000000000c00

Changes to memory:
0x01f0: 0x0000000000000000      0x000000000000005b
0x01f8: 0x0000000000000000      0x0000000000000013
```

This is `rsum.ys`:

```asm
# Execution begins at address 0
.pos 0
irmovq stack, %rsp # Set up stack pointer
call main # Execute main program
halt # Terminate program

# Test linked list
.align 8
ele1:
  .quad 0x00a
  .quad ele2
ele2:
  .quad 0x0b0
  .quad ele3
ele3:
  .quad 0xc00
  .quad 0

main:
   irmovq ele1, %rdi
   call rsum_list # rsum_list(ele1)
   ret

# long rsum_list(list_ptr ls)
# ls in %rdi
rsum_list:
  andq %rdi, %rdi # Set CC
  jne recurise
  xorq %rax, %rax # val = 0
  ret
recurise:
  mrmovq (%rdi), %r10 # Get ls->val
  mrmovq 0x8(%rdi), %rdi # Get ls->next
  pushq %r10
  call rsum_list # Recurise
  popq %r10
  addq %r10, %rax # Add to val
  ret

# Stack starts here and grows to lower address
.pos 0x200
stack:
```

This is the result after `./yas rsum.ys` and `./yis rsum.yo`:

```txt
Stopped in 37 steps at PC = 0x13.  Status 'HLT', CC Z=0 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rsp:   0x0000000000000000      0x0000000000000200
%r10:   0x0000000000000000      0x000000000000000a

Changes to memory:
0x01c0: 0x0000000000000000      0x0000000000000089
0x01c8: 0x0000000000000000      0x0000000000000c00
0x01d0: 0x0000000000000000      0x0000000000000089
0x01d8: 0x0000000000000000      0x00000000000000b0
0x01e0: 0x0000000000000000      0x0000000000000089
0x01e8: 0x0000000000000000      0x000000000000000a
0x01f0: 0x0000000000000000      0x000000000000005b
0x01f8: 0x0000000000000000      0x0000000000000013
```

This is `copy.ys`:

```asm
# Execution begins at address 0
.pos 0
irmovq stack, %rsp # Set up stack pointer
call main # Execute main program
halt # Terminate program

# Test block
# Source block
src:
  .quad 0x00a
  .quad 0x0b0
  .quad 0xc00
# Destination block
dest:
  .quad 0x111
  .quad 0x222
  .quad 0x333

main:
   irmovq src, %rdi
   irmovq dest, %rsi
   irmovq $0x3, %rdx
   call copy_block # copy_block(src, dest, 3)
   ret

# long copy_block(long *src, long *dest, long len)
# src in %rdi, dest in %rsi, len in %rdx
copy_block:
  xorq %rax, %rax # result = 0
  andq %rdx, %rdx # Set CC
  jmp test
loop:
  mrmovq (%rdi), %r8 # Get *src
  irmovq $0x8, %r9
  addq %r9, %rdi # src++
  rmmovq %r8, (%rsi) # Set *dest
  addq %r9, %rsi # dest++
  xorq %r8, %rax # Checksum
  irmovq $0x1, %r10
  subq %r10, %rdx # len--
  andq %rdx, %rdx # Set CC
test:
  jne loop
  ret

# Stack starts here and grows to lower address
.pos 0x200
stack:

```

This is the result after `./yas copy.ys` and `./yis copy.yo`:

```txt
Stopped in 43 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rsp:   0x0000000000000000      0x0000000000000200
%rsi:   0x0000000000000000      0x0000000000000044
%rdi:   0x0000000000000000      0x000000000000002c
%r8:    0x0000000000000000      0x0000000000000c00
%r9:    0x0000000000000000      0x0000000000000008
%r10:   0x0000000000000000      0x0000000000000001

Changes to memory:
0x0028: 0x0000011100000000      0x0000000a00000000
0x0030: 0x0000022200000000      0x000000b000000000
0x0038: 0x0000033300000000      0x00000c0000000000
0x01f0: 0x0000000000000000      0x000000000000006b
0x01f8: 0x0000000000000000      0x0000000000000013
```

## Part B

We can mimic the descriptions of `irmovq` and `OPq` in Figure 4.18 in the book to describe stages for `iaddq`:

```txt
# Stages for iaddq V, rB: add constant V to rB
#
# * Fetch
#   * icode:ifun <- M1[PC]
#   * rA:rB <- M1[PC+1]
#   * valC <- M8[PC+2]
#   * valP <- PC+10
# * Decode
#   * valB <- R[rB]
# * Execute
#   * valE <- valC+valB
#   * Set CC
# * Memory
# * Write Back
#   * R[rB] <- valE
# * PC Update
#   * PC <- valP
```

Note that we need to set conditional code when the processor executes the instruction.

Described above, we modify the `seq-full.hcl` file, adding `IIADDQ` into stages to implement it.

To test the solution, we change the value of `VERSION` to full, and comment out options referring to GUI mode. Then, we use `make` to build a new SEQ simulator.

Described with `archlab.pdf`, we test the solution using a simple Y86-64 program, the benchmark programs, and regression tests. The results are shown below:

```txt
Y86-64 Processor: seq-full.hcl
137 bytes of code read
IF: Fetched irmovq at 0x0.  ra=----, rb=%rsp, valC = 0x100
IF: Fetched call at 0xa.  ra=----, rb=----, valC = 0x38
Wrote 0x13 to address 0xf8
IF: Fetched irmovq at 0x38.  ra=----, rb=%rdi, valC = 0x18
IF: Fetched irmovq at 0x42.  ra=----, rb=%rsi, valC = 0x4
IF: Fetched call at 0x4c.  ra=----, rb=----, valC = 0x56
Wrote 0x55 to address 0xf0
IF: Fetched xorq at 0x56.  ra=%rax, rb=%rax, valC = 0x0
IF: Fetched andq at 0x58.  ra=%rsi, rb=%rsi, valC = 0x0
IF: Fetched jmp at 0x5a.  ra=----, rb=----, valC = 0x83
IF: Fetched jne at 0x83.  ra=----, rb=----, valC = 0x63
IF: Fetched mrmovq at 0x63.  ra=%r10, rb=%rdi, valC = 0x0
IF: Fetched addq at 0x6d.  ra=%r10, rb=%rax, valC = 0x0
IF: Fetched iaddq at 0x6f.  ra=----, rb=%rdi, valC = 0x8
IF: Fetched iaddq at 0x79.  ra=----, rb=%rsi, valC = 0xffffffffffffffff
IF: Fetched jne at 0x83.  ra=----, rb=----, valC = 0x63
IF: Fetched mrmovq at 0x63.  ra=%r10, rb=%rdi, valC = 0x0
IF: Fetched addq at 0x6d.  ra=%r10, rb=%rax, valC = 0x0
IF: Fetched iaddq at 0x6f.  ra=----, rb=%rdi, valC = 0x8
IF: Fetched iaddq at 0x79.  ra=----, rb=%rsi, valC = 0xffffffffffffffff
IF: Fetched jne at 0x83.  ra=----, rb=----, valC = 0x63
IF: Fetched mrmovq at 0x63.  ra=%r10, rb=%rdi, valC = 0x0
IF: Fetched addq at 0x6d.  ra=%r10, rb=%rax, valC = 0x0
IF: Fetched iaddq at 0x6f.  ra=----, rb=%rdi, valC = 0x8
IF: Fetched iaddq at 0x79.  ra=----, rb=%rsi, valC = 0xffffffffffffffff
IF: Fetched jne at 0x83.  ra=----, rb=----, valC = 0x63
IF: Fetched mrmovq at 0x63.  ra=%r10, rb=%rdi, valC = 0x0
IF: Fetched addq at 0x6d.  ra=%r10, rb=%rax, valC = 0x0
IF: Fetched iaddq at 0x6f.  ra=----, rb=%rdi, valC = 0x8
IF: Fetched iaddq at 0x79.  ra=----, rb=%rsi, valC = 0xffffffffffffffff
IF: Fetched jne at 0x83.  ra=----, rb=----, valC = 0x63
IF: Fetched ret at 0x8c.  ra=----, rb=----, valC = 0x0
IF: Fetched ret at 0x55.  ra=----, rb=----, valC = 0x0
IF: Fetched halt at 0x13.  ra=----, rb=----, valC = 0x0
32 instructions executed
Status = HLT
Condition Codes: Z=1 S=0 O=0
Changed Register State:
%rax:   0x0000000000000000      0x0000abcdabcdabcd
%rsp:   0x0000000000000000      0x0000000000000100
%rdi:   0x0000000000000000      0x0000000000000038
%r10:   0x0000000000000000      0x0000a000a000a000
Changed Memory State:
0x00f0: 0x0000000000000000      0x0000000000000055
0x00f8: 0x0000000000000000      0x0000000000000013
ISA Check Succeeds

Makefile:42: warning: ignoring prerequisites on suffix rule definition
Makefile:45: warning: ignoring prerequisites on suffix rule definition
Makefile:48: warning: ignoring prerequisites on suffix rule definition
Makefile:51: warning: ignoring prerequisites on suffix rule definition
../seq/ssim -t asum.yo > asum.seq
../seq/ssim -t asumr.yo > asumr.seq
../seq/ssim -t cjr.yo > cjr.seq
../seq/ssim -t j-cc.yo > j-cc.seq
../seq/ssim -t poptest.yo > poptest.seq
../seq/ssim -t pushquestion.yo > pushquestion.seq
../seq/ssim -t pushtest.yo > pushtest.seq
../seq/ssim -t prog1.yo > prog1.seq
../seq/ssim -t prog2.yo > prog2.seq
../seq/ssim -t prog3.yo > prog3.seq
../seq/ssim -t prog4.yo > prog4.seq
../seq/ssim -t prog5.yo > prog5.seq
../seq/ssim -t prog6.yo > prog6.seq
../seq/ssim -t prog7.yo > prog7.seq
../seq/ssim -t prog8.yo > prog8.seq
../seq/ssim -t ret-hazard.yo > ret-hazard.seq
grep "ISA Check" *.seq
asum.seq:ISA Check Succeeds
asumr.seq:ISA Check Succeeds
cjr.seq:ISA Check Succeeds
j-cc.seq:ISA Check Succeeds
poptest.seq:ISA Check Succeeds
prog1.seq:ISA Check Succeeds
prog2.seq:ISA Check Succeeds
prog3.seq:ISA Check Succeeds
prog4.seq:ISA Check Succeeds
prog5.seq:ISA Check Succeeds
prog6.seq:ISA Check Succeeds
prog7.seq:ISA Check Succeeds
prog8.seq:ISA Check Succeeds
pushquestion.seq:ISA Check Succeeds
pushtest.seq:ISA Check Succeeds
ret-hazard.seq:ISA Check Succeeds
rm asum.seq asumr.seq cjr.seq j-cc.seq poptest.seq pushquestion.seq pushtest.seq prog1.seq prog2.seq prog3.seq prog4.seq prog5.seq prog6.seq prog7.seq prog8.seq ret-hazard.seq

./optest.pl -s ../seq/ssim
Simulating with ../seq/ssim
  All 49 ISA Checks Succeed
./jtest.pl -s ../seq/ssim
Simulating with ../seq/ssim
  All 64 ISA Checks Succeed
./ctest.pl -s ../seq/ssim
Simulating with ../seq/ssim
  All 22 ISA Checks Succeed
./htest.pl -s ../seq/ssim
Simulating with ../seq/ssim
  All 600 ISA Checks Succeed

./optest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 58 ISA Checks Succeed
./jtest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 96 ISA Checks Succeed
./ctest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 22 ISA Checks Succeed
./htest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 756 ISA Checks Succeed
```

## Part C

The sentence in `archlab.pdf`, "You may make any semantics preserving transformations to the `ncopy.ys` function, such as reordering instruc- tions, replacing groups of instructions with single instructions, deleting some instructions, and adding other instructions. You may find it useful to read about loop unrolling in Section 5.8 of CS:APP3e.", gives us lots of inspiration.

Firstly, we can replace the `irmovq` instruction combined with `addq` or `subq`, with single `iaddq` instruction. We add `iaddq` into `pipe-full.hcl` by the similar way in part B. The replacement can reduce one instruction per calculation.

So that the program looks like this:

```asm
ncopy:
 xorq %rax,%rax  # count = 0;
 andq %rdx,%rdx  # len <= 0?
 jle Done  # if so, goto Done:
Loop:
  mrmovq (%rdi), %r10 # read val from src...
 rmmovq %r10, (%rsi) # ...and store it to dst
 andq %r10, %r10  # val <= 0?
 jle Npos  # if so, goto Npos:
  iaddq $1, %rax # count++
Npos:
  iaddq $-1, %rdx # len--
  iaddq $1, %rdi # src++
  iaddq $1, %rsi # dst++
 andq %rdx,%rdx  # len > 0?
 jg Loop   # if so, goto Loop:
Done:
 ret
```

We use loop unrolling to further optimize performance. We choose a loop unrolling factor of 8 and use the binary search method to determine the remainder. Therefore, the remainder can be determined with a maximum of only three operations.

So that the program looks like this:

```asm
ncopy:
  xorq %rax, %rax # count = 0
  iaddq $-8, %rdx
  jl handle_remainder # 0 < len < 8
loop:
  mrmovq (%rdi), %rcx
  mrmovq 0x8(%rdi), %r8
  mrmovq 0x10(%rdi), %r9
  mrmovq 0x18(%rdi), %r10
  mrmovq 0x20(%rdi), %r11
  mrmovq 0x28(%rdi), %r12
  mrmovq 0x30(%rdi), %r13
  mrmovq 0x38(%rdi), %r14
  rmmovq %rcx, (%rsi)
  rmmovq %r8, 0x8(%rsi)
  rmmovq %r9, 0x10(%rsi)
  rmmovq %r10, 0x18(%rsi)
  rmmovq %r11, 0x20(%rsi)
  rmmovq %r12, 0x28(%rsi)
  rmmovq %r13, 0x30(%rsi)
  rmmovq %r14, 0x38(%rsi)
# if the 1st value > 0?
check_1st:
  andq %rcx, %rcx
  jle check_2nd
  iaddq $1, %rax
# if the 2nd value > 0?
check_2nd:
  andq %r8, %r8
  jle check_3rd
  iaddq $1, %rax
# if the 3rd value > 0?
check_3rd:
  andq %r9, %r9
  jle check_4th
  iaddq $1, %rax
# if the 4th value > 0?
check_4th:
  andq %r10, %r10
  jle check_5th
  iaddq $1, %rax
# if the 5th value > 0?
check_5th:
  andq %r11, %r11
  jle check_6th
  iaddq $1, %rax
# if the 6th value > 0?
check_6th:
  andq %r12, %r12
  jle check_7th
  iaddq $1, %rax
# if the 7th value > 0?
check_7th:
  andq %r13, %r13
  jle check_8th
  iaddq $1, %rax
# if the 8th value > 0?
check_8th:
  andq %r14, %r14
  jle npos
  iaddq $1, %rax
npos:
  iaddq $64, %rdi
  iaddq $64, %rsi
  iaddq $-8, %rdx
  jge loop
# rdx range from -8 to -1
handle_remainder:
  iaddq $4, %rdx
  jl remainder_0_3
# rdx >= -4 -> rdx+8 >= 4 -> remainder is [4,7]
remainder_4_7:
  iaddq $-2, %rdx
  jl remainder_4_5
# rdx+4 >= 2 -> rdx+8 >= 6 -> remainder is [6,7]
remainder_6_7:
  iaddq $-1, %rdx
# rdx+2 < 1 -> rdx+8 < 7 -> remainder is 6
  jl remainder_6
# rdx+2 >= 1 -> rdx+8 >= 7 -> remainder is 7
  jmp remainder_7
# rdx < -4 -> rdx+8 < 4 -> remainder is [0,3]
remainder_0_3:
  iaddq $2, %rdx
  jl remainder_0_1
# rdx+4 >= -2 -> rdx+8 >= 2 -> remainder is [2,3]
remainder_2_3:
  iaddq $-1, %rdx
# rdx+6 < 1 -> rdx+8 < 3 -> remainder is 2
  jl remainder_2
# rdx+6 >= 1 -> rdx+8 >= 3 -> remainder is 3
  jmp remainder_3
# rdx+4 < -2 -> rdx+8 < 2 -> remainder is [0,1]
remainder_0_1:
  iaddq $1, %rdx
# rdx+6 < -1 -> rdx+8 < 1 -> remainder is 0
  jl Done
# rdx+6 >= -1 -> rdx+8 >= 1 -> remainder is 1
  jmp remainder_1
# rdx+4 < 2 -> rdx+8 < 6 -> remainder is [4,5]
remainder_4_5:
  iaddq $1, %rdx
# rdx+2 < -1 -> rdx+8 < 5 -> remainder is 4
  jl remainder_4
# rdx+2 >= -1 -> rdx+8 >= 5 -> remainder is 5
  jmp remainder_5
remainder_7:
  mrmovq 48(%rdi), %rbx
  rmmovq %rbx, 48(%rsi)
  andq %rbx, %rbx
  jle remainder_6
  iaddq $1, %rax
remainder_6:
  mrmovq 40(%rdi), %rbx
  rmmovq %rbx, 40(%rsi)
  andq %rbx, %rbx
  jle remainder_5
  iaddq $1, %rax
remainder_5:
  mrmovq 32(%rdi), %rbx
  rmmovq %rbx, 32(%rsi)
  andq %rbx, %rbx
  jle remainder_4
  iaddq $1, %rax
remainder_4:
  mrmovq 24(%rdi), %rbx
  rmmovq %rbx, 24(%rsi)
  andq %rbx, %rbx
  jle remainder_3
  iaddq $1, %rax
remainder_3:
  mrmovq 16(%rdi), %rbx
  rmmovq %rbx, 16(%rsi)
  andq %rbx, %rbx
  jle remainder_2
  iaddq $1, %rax
remainder_2:
  mrmovq 8(%rdi), %rbx
  rmmovq %rbx, 8(%rsi)
  andq %rbx, %rbx
  jle remainder_1
  iaddq $1, %rax
remainder_1:
  mrmovq (%rdi), %rbx
  rmmovq %rbx, (%rsi)
  andq %rbx, %rbx
  jle Done
  iaddq $1, %rax
Done:
 ret
```

We found that using `mrmovq` followed by `rmmovq`, which operates the same memory address, will cause load/use data hazard:

```txt
...
mrmovq 48(%rdi), %rbx
rmmovq %rbx, 48(%rsi)
...
```

This will cause one cycle wasted. Therefore, we can use this cycle to execute other instruction. The modified code is shown below:

```asm
ncopy:
  xorq %rax, %rax # count = 0
  iaddq $-8, %rdx
  jl handle_remainder # 0 < len < 8
loop:
  mrmovq (%rdi), %rcx
copy_and_check_1st:
  mrmovq 0x8(%rdi), %r8
  rmmovq %rcx, (%rsi)
# if the 1st value > 0?
  andq %rcx, %rcx
  jle copy_and_check_2nd
  iaddq $1, %rax
copy_and_check_2nd:
  mrmovq 0x10(%rdi), %r9
  rmmovq %r8, 0x8(%rsi)
# if the 2nd value > 0?
  andq %r8, %r8
  jle copy_and_check_3rd
  iaddq $1, %rax
copy_and_check_3rd:
  mrmovq 0x18(%rdi), %r10
  rmmovq %r9, 0x10(%rsi)
# if the 3rd value > 0?
  andq %r9, %r9
  jle copy_and_check_4th
  iaddq $1, %rax
copy_and_check_4th:
  mrmovq 0x20(%rdi), %r11
  rmmovq %r10, 0x18(%rsi)
# if the 4th value > 0?
  andq %r10, %r10
  jle copy_and_check_5th
  iaddq $1, %rax
copy_and_check_5th:
  mrmovq 0x28(%rdi), %r12
  rmmovq %r11, 0x20(%rsi)
# if the 5th value > 0?
  andq %r11, %r11
  jle copy_and_check_6th
  iaddq $1, %rax
copy_and_check_6th:
  mrmovq 0x30(%rdi), %r13
  rmmovq %r12, 0x28(%rsi)
# if the 6th value > 0?
  andq %r12, %r12
  jle copy_and_check_7th
  iaddq $1, %rax
copy_and_check_7th:
  mrmovq 0x38(%rdi), %r14
  rmmovq %r13, 0x30(%rsi)
# if the 7th value > 0?
  andq %r13, %r13
  jle copy_and_check_8th
  iaddq $1, %rax
copy_and_check_8th:
  rmmovq %r14, 0x38(%rsi)
# if the 8th value > 0?
  andq %r14, %r14
  jle npos
  iaddq $1, %rax
npos:
  iaddq $64, %rdi
  iaddq $64, %rsi
  iaddq $-8, %rdx
  jge loop
# rdx range from -8 to -1
handle_remainder:
  iaddq $4, %rdx
  jl remainder_0_3
# rdx >= -4 -> rdx+8 >= 4 -> remainder is [4,7]
remainder_4_7:
  iaddq $-2, %rdx
  jl remainder_4_5
# rdx+4 >= 2 -> rdx+8 >= 6 -> remainder is [6,7]
remainder_6_7:
  iaddq $-1, %rdx
# rdx+2 < 1 -> rdx+8 < 7 -> remainder is 6
  mrmovq 40(%rdi), %rbx
  jl remainder_6
# rdx+2 >= 1 -> rdx+8 >= 7 -> remainder is 7
  mrmovq 48(%rdi), %rbx
  jmp remainder_7
# rdx < -4 -> rdx+8 < 4 -> remainder is [0,3]
remainder_0_3:
  iaddq $2, %rdx
  jl remainder_0_1
# rdx+4 >= -2 -> rdx+8 >= 2 -> remainder is [2,3]
remainder_2_3:
  iaddq $-1, %rdx
# rdx+6 < 1 -> rdx+8 < 3 -> remainder is 2
  mrmovq 8(%rdi), %rbx
  jl remainder_2
# rdx+6 >= 1 -> rdx+8 >= 3 -> remainder is 3
  mrmovq 16(%rdi), %rbx
  jmp remainder_3
# rdx+4 < -2 -> rdx+8 < 2 -> remainder is [0,1]
remainder_0_1:
  iaddq $1, %rdx
# rdx+6 < -1 -> rdx+8 < 1 -> remainder is 0
  jl Done
# rdx+6 >= -1 -> rdx+8 >= 1 -> remainder is 1
  mrmovq (%rdi), %rbx
  jmp remainder_1
# rdx+4 < 2 -> rdx+8 < 6 -> remainder is [4,5]
remainder_4_5:
  iaddq $1, %rdx
# rdx+2 < -1 -> rdx+8 < 5 -> remainder is 4
  mrmovq 24(%rdi), %rbx
  jl remainder_4
# rdx+2 >= -1 -> rdx+8 >= 5 -> remainder is 5
  mrmovq 32(%rdi), %rbx
  jmp remainder_5
remainder_7:
  rmmovq %rbx, 48(%rsi)
  andq %rbx, %rbx
  mrmovq 40(%rdi), %rbx
  jle remainder_6
  iaddq $1, %rax
remainder_6:
  rmmovq %rbx, 40(%rsi)
  andq %rbx, %rbx
  mrmovq 32(%rdi), %rbx
  jle remainder_5
  iaddq $1, %rax
remainder_5:
  rmmovq %rbx, 32(%rsi)
  andq %rbx, %rbx
  mrmovq 24(%rdi), %rbx
  jle remainder_4
  iaddq $1, %rax
remainder_4:
  rmmovq %rbx, 24(%rsi)
  andq %rbx, %rbx
  mrmovq 16(%rdi), %rbx
  jle remainder_3
  iaddq $1, %rax
remainder_3:
  rmmovq %rbx, 16(%rsi)
  andq %rbx, %rbx
  mrmovq 8(%rdi), %rbx
  jle remainder_2
  iaddq $1, %rax
remainder_2:
  rmmovq %rbx, 8(%rsi)
  andq %rbx, %rbx
  mrmovq (%rdi), %rbx
  jle remainder_1
  iaddq $1, %rax
remainder_1:
  rmmovq %rbx, (%rsi)
  andq %rbx, %rbx
  jle Done
  iaddq $1, %rax
Done:
 ret
```

In order to test the solution, we build the driver programs and rebuild the simulator by modifying the `Makefile`. We modify the `VERSION` to `full` and comment out options related to GUIMODE.

After typing `make`, we test our code on a range of block lengths with the ISA simulator by `./correctness.pl`. The result is:

```txt
68/68 pass correctness test
```

Finally, we run the command `benchmark`, and the result is:

```txt
        ncopy
0       19
1       26      26.00
2       32      16.00
3       40      13.33
4       43      10.75
5       51      10.20
6       56      9.33
7       64      9.14
8       71      8.88
9       78      8.67
10      84      8.40
11      92      8.36
12      95      7.92
13      103     7.92
14      108     7.71
15      116     7.73
16      119     7.44
17      126     7.41
18      132     7.33
19      140     7.37
20      143     7.15
21      151     7.19
22      156     7.09
23      164     7.13
24      167     6.96
25      174     6.96
26      180     6.92
27      188     6.96
28      191     6.82
29      199     6.86
30      204     6.80
31      212     6.84
32      215     6.72
33      222     6.73
34      228     6.71
35      236     6.74
36      239     6.64
37      247     6.68
38      252     6.63
39      260     6.67
40      263     6.58
41      270     6.59
42      276     6.57
43      284     6.60
44      287     6.52
45      295     6.56
46      300     6.52
47      308     6.55
48      311     6.48
49      318     6.49
50      324     6.48
51      332     6.51
52      335     6.44
53      343     6.47
54      348     6.44
55      356     6.47
56      359     6.41
57      366     6.42
58      372     6.41
59      380     6.44
60      383     6.38
61      391     6.41
62      396     6.39
63      404     6.41
64      407     6.36
Average CPE     7.64
Score   57.2/60.0
```

If you want to further optimize performance, you can refer to [this article](https://github.com/zhuozhiyongde/Introduction-to-Computer-System-2023Fall-PKU/tree/main/04-Arch-Lab) for more information.

Till now, we have completed arch lab.
