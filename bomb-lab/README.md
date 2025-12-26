# Solution Process

## Basis

1. Use `lscpu` to get CPU info in Linux:

```shell
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          43 bits physical, 48 bits virtual
  Byte Order:             Little Endian
```

2. Use `cat /etc/os-release` to get OS info in Linux:

```shell
PRETTY_NAME="Ubuntu 24.04.3 LTS"
NAME="Ubuntu"
VERSION_ID="24.04"
VERSION="24.04.3 LTS (Noble Numbat)"
VERSION_CODENAME=noble
```

3. Use `objdump -v` and `gdb -v` to get their info:

```shell
GNU objdump (GNU Binutils for Ubuntu) 2.42

GNU gdb (Ubuntu 15.0.50.20240403-0ubuntu1) 15.0.50.20240403-git
```

## Process

Read the `bomb.c` file, and we can figure out that the key point is shown below, which inspires us to find out the logic of procedures of `phase_x(input)` to defuse the bomb.

```c
  input = read_line();
  phase_1(input);
  phase_defused();
```

Use `objdump -d bomb > bomb.asm` to disassemble `bomb` to assembly file, and we can observe different logic of six phases, as well as a secret phase. To check register or memory values, we need `gdb`, and here are some useful commands.

| Command |                   Description                    |
| :------ | :----------------------------------------------: |
| h       |        Get help on a specific gdb command        |
| r       |         Run to next breakpoint or to end         |
| s       |      Single-step, descending into functions      |
| n       |  Single-step, without descending into functions  |
| c       |      Continue to next breakpoint or to end       |
| up      |         Go up one context level on stack         |
| do      |   Go down one level (only possible after _up_)   |
| l       | Show lines of code surrounding the current point |
| p       |             Print value of variable              |
| x       |              Print value in memory               |
| j       |              Jump to specified line              |
| b       |                 Set a breakpoint                 |
| i b     |                 List breakpoints                 |
| dis 1   |               Disable breakpoint 1               |
| en 1    |               Enable breakpoint 1                |
| d 1     |               Delete breakpoint 1                |
| layout  |              Output values or code               |
| q       |                     Quit gdb                     |

Use `touch code` to add a file, which records codes for different phases. Use `gdb --args bomb code` to start debugging the program.

### Phase 1

The relevant logic for phase 1 is shown below:

```asm
0000000000400da0 <main>:
...
  400e32: e8 67 06 00 00        call   40149e <read_line>
  400e37: 48 89 c7              mov    %rax,%rdi
  400e3a: e8 a1 00 00 00        call   400ee0 <phase_1>
...

0000000000400ee0 <phase_1>:
  400ee0: 48 83 ec 08           sub    $0x8,%rsp
  400ee4: be 00 24 40 00        mov    $0x402400,%esi
  400ee9: e8 4a 04 00 00        call   401338 <strings_not_equal>
  400eee: 85 c0                 test   %eax,%eax
  400ef0: 74 05                 je     400ef7 <phase_1+0x17>
  400ef2: e8 43 05 00 00        call   40143a <explode_bomb>
  400ef7: 48 83 c4 08           add    $0x8,%rsp
  400efb: c3                    ret
```

Overall, the `main` gets one-line string from `code`, and then pass it to `phase_1` as the 1st argument. In `phase_1`, it gets a string, which the address of the first character is `0x402400`, and treat it as the 2nd argument, passing it to the `strings_not_equal` procedure with the 1st argument passed by caller `main`.
By using `x/s $rsi`, we can get the answer we want. Phase 1 completed.

```asm
0x402400:       "Border relations with Canada have never been better."
```

### Phase 2

The relevant logic for phase 2 is shown below:

```asm
0000000000400da0 <main>:
...
  400e4e: e8 4b 06 00 00        call   40149e <read_line>
  400e53: 48 89 c7              mov    %rax,%rdi
  400e56: e8 a1 00 00 00        call   400efc <phase_2>
...

0000000000400efc <phase_2>:
  400efc: 55                    push   %rbp
  400efd: 53                    push   %rbx
  400efe: 48 83 ec 28           sub    $0x28,%rsp
  400f02: 48 89 e6              mov    %rsp,%rsi
  400f05: e8 52 05 00 00        call   40145c <read_six_numbers>
  400f0a: 83 3c 24 01           cmpl   $0x1,(%rsp)
  400f0e: 74 20                 je     400f30 <phase_2+0x34>
  400f10: e8 25 05 00 00        call   40143a <explode_bomb>
  400f15: eb 19                 jmp    400f30 <phase_2+0x34>
  400f17: 8b 43 fc              mov    -0x4(%rbx),%eax
  400f1a: 01 c0                 add    %eax,%eax
  400f1c: 39 03                 cmp    %eax,(%rbx)
  400f1e: 74 05                 je     400f25 <phase_2+0x29>
  400f20: e8 15 05 00 00        call   40143a <explode_bomb>
  400f25: 48 83 c3 04           add    $0x4,%rbx
  400f29: 48 39 eb              cmp    %rbp,%rbx
  400f2c: 75 e9                 jne    400f17 <phase_2+0x1b>
  400f2e: eb 0c                 jmp    400f3c <phase_2+0x40>
  400f30: 48 8d 5c 24 04        lea    0x4(%rsp),%rbx
  400f35: 48 8d 6c 24 18        lea    0x18(%rsp),%rbp
  400f3a: eb db                 jmp    400f17 <phase_2+0x1b>
  400f3c: 48 83 c4 28           add    $0x28,%rsp
  400f40: 5b                    pop    %rbx
  400f41: 5d                    pop    %rbp
  400f42: c3                    ret
```

The logic is a little bit verbose, so I translate it into c-like pseudocode:

```c
main() {
  ...
  char *input = read_line();
  phase_2(input);
  ...
}

phase_2(char *input) {
  int_32 nums[6];
  read_six_numbers(input, nums);
  if (*(nums) != 0x1) {
    explode_bomb();
  }
  int_32* tmp1 = nums + 1;
  int_32* tmp2 = nums + 6;
  do {
    int_32 tmp3 = *(tmp1 - 1);
    tmp3 <<= 1;
    if (tmp3 != *tmp1) explode_bomb();
    tmp1 += 1;
  } while (tmp1 != tmp2);
}

read_six_numbers(char *input, int_32* nums) {
  int_32 res = __isoc99_sscanf(input, "%d %d %d %d %d %d", nums, nums + 1, nums + 2, nums + 3, nums + 4, nums + 5);
  if (res <= 5) explode_bomb();
}
```

The answer consists of six numbers, each number being twice the previous one and starting at 1.
So the answer is:

```txt
1 2 4 8 16 32
```

### Phase 3

The relevant logic for phase 3 is shown below:

```asm
0000000000400f43 <phase_3>:
  400f43: 48 83 ec 18           sub    $0x18,%rsp
  400f47: 48 8d 4c 24 0c        lea    0xc(%rsp),%rcx
  400f4c: 48 8d 54 24 08        lea    0x8(%rsp),%rdx
  400f51: be cf 25 40 00        mov    $0x4025cf,%esi
  400f56: b8 00 00 00 00        mov    $0x0,%eax
  400f5b: e8 90 fc ff ff        call   400bf0 <__isoc99_sscanf@plt>
  400f60: 83 f8 01              cmp    $0x1,%eax
  400f63: 7f 05                 jg     400f6a <phase_3+0x27>
  400f65: e8 d0 04 00 00        call   40143a <explode_bomb>
  400f6a: 83 7c 24 08 07        cmpl   $0x7,0x8(%rsp)
  400f6f: 77 3c                 ja     400fad <phase_3+0x6a>
  400f71: 8b 44 24 08           mov    0x8(%rsp),%eax
  400f75: ff 24 c5 70 24 40 00  jmp    *0x402470(,%rax,8)
  400f7c: b8 cf 00 00 00        mov    $0xcf,%eax
  400f81: eb 3b                 jmp    400fbe <phase_3+0x7b>
  400f83: b8 c3 02 00 00        mov    $0x2c3,%eax
  400f88: eb 34                 jmp    400fbe <phase_3+0x7b>
  400f8a: b8 00 01 00 00        mov    $0x100,%eax
  400f8f: eb 2d                 jmp    400fbe <phase_3+0x7b>
  400f91: b8 85 01 00 00        mov    $0x185,%eax
  400f96: eb 26                 jmp    400fbe <phase_3+0x7b>
  400f98: b8 ce 00 00 00        mov    $0xce,%eax
  400f9d: eb 1f                 jmp    400fbe <phase_3+0x7b>
  400f9f: b8 aa 02 00 00        mov    $0x2aa,%eax
  400fa4: eb 18                 jmp    400fbe <phase_3+0x7b>
  400fa6: b8 47 01 00 00        mov    $0x147,%eax
  400fab: eb 11                 jmp    400fbe <phase_3+0x7b>
  400fad: e8 88 04 00 00        call   40143a <explode_bomb>
  400fb2: b8 00 00 00 00        mov    $0x0,%eax
  400fb7: eb 05                 jmp    400fbe <phase_3+0x7b>
  400fb9: b8 37 01 00 00        mov    $0x137,%eax
  400fbe: 3b 44 24 0c           cmp    0xc(%rsp),%eax
  400fc2: 74 05                 je     400fc9 <phase_3+0x86>
  400fc4: e8 71 04 00 00        call   40143a <explode_bomb>
  400fc9: 48 83 c4 18           add    $0x18,%rsp
  400fcd: c3                    ret
```

The corresponding logic in c-like pseudocode is shown below:

```c
phase_3(char *input) {
  int_32 n1;
  int_32 n2;
  int_32 res = __isoc99_sscanf(input, "%d %d", &n1, &n2);
  if (res <= 0x1) explode_bomb();
  if (((uint_32)n1) > 0x7) explode_bomb();
  switch (n1) {
    case 0:
      res = 0xcf;
      break;
    case 1:
      res = 0x137;
      break;
    case 2:
      res = 0x2c3;
      break;
    case 3:
      res = 0x100;
      break;
    case 4:
      res = 0x185;
      break;
    case 5:
      res = 0xce;
      break;
    case 6:
      res = 0x2aa;
      break;
    case 7:
      res = 0x147;
      break;
  }
  if (n2 != res) explode_bomb();
}
```

We can use `x 0x402470` ~ `x 0x402470 + 8 * 7` to display the address in jump table, and converse them into switch-case form. So, there are 8 legal answers. I chose `0 207` as my answer.

### Phase 4

The relevant logic for phase 4 is shown below:

```asm
0000000000400fce <func4>:
  400fce: 48 83 ec 08           sub    $0x8,%rsp
  400fd2: 89 d0                 mov    %edx,%eax
  400fd4: 29 f0                 sub    %esi,%eax
  400fd6: 89 c1                 mov    %eax,%ecx
  400fd8: c1 e9 1f              shr    $0x1f,%ecx
  400fdb: 01 c8                 add    %ecx,%eax
  400fdd: d1 f8                 sar    $1,%eax
  400fdf: 8d 0c 30              lea    (%rax,%rsi,1),%ecx
  400fe2: 39 f9                 cmp    %edi,%ecx
  400fe4: 7e 0c                 jle    400ff2 <func4+0x24>
  400fe6: 8d 51 ff              lea    -0x1(%rcx),%edx
  400fe9: e8 e0 ff ff ff        call   400fce <func4>
  400fee: 01 c0                 add    %eax,%eax
  400ff0: eb 15                 jmp    401007 <func4+0x39>
  400ff2: b8 00 00 00 00        mov    $0x0,%eax
  400ff7: 39 f9                 cmp    %edi,%ecx
  400ff9: 7d 0c                 jge    401007 <func4+0x39>
  400ffb: 8d 71 01              lea    0x1(%rcx),%esi
  400ffe: e8 cb ff ff ff        call   400fce <func4>
  401003: 8d 44 00 01           lea    0x1(%rax,%rax,1),%eax
  401007: 48 83 c4 08           add    $0x8,%rsp
  40100b: c3                    ret

000000000040100c <phase_4>:
  40100c: 48 83 ec 18           sub    $0x18,%rsp
  401010: 48 8d 4c 24 0c        lea    0xc(%rsp),%rcx
  401015: 48 8d 54 24 08        lea    0x8(%rsp),%rdx
  40101a: be cf 25 40 00        mov    $0x4025cf,%esi
  40101f: b8 00 00 00 00        mov    $0x0,%eax
  401024: e8 c7 fb ff ff        call   400bf0 <__isoc99_sscanf@plt>
  401029: 83 f8 02              cmp    $0x2,%eax
  40102c: 75 07                 jne    401035 <phase_4+0x29>
  40102e: 83 7c 24 08 0e        cmpl   $0xe,0x8(%rsp)
  401033: 76 05                 jbe    40103a <phase_4+0x2e>
  401035: e8 00 04 00 00        call   40143a <explode_bomb>
  40103a: ba 0e 00 00 00        mov    $0xe,%edx
  40103f: be 00 00 00 00        mov    $0x0,%esi
  401044: 8b 7c 24 08           mov    0x8(%rsp),%edi
  401048: e8 81 ff ff ff        call   400fce <func4>
  40104d: 85 c0                 test   %eax,%eax
  40104f: 75 07                 jne    401058 <phase_4+0x4c>
  401051: 83 7c 24 0c 00        cmpl   $0x0,0xc(%rsp)
  401056: 74 05                 je     40105d <phase_4+0x51>
  401058: e8 dd 03 00 00        call   40143a <explode_bomb>
  40105d: 48 83 c4 18           add    $0x18,%rsp
  401061: c3                    ret
```

The corresponding logic in c-like pseudocode is shown below:

```c
phase_4(char* input) {
  int_32 n1;
  int_32 n2;
  int_32 res = __isoc99_sscanf(input, "%d %d", &n1, &n2);
  if (res != 0x2) explode_bomb();
  if (((uint_32)n1) > 0xe) explode_bomb();
  res = func4(n1, 0x0, 0xe);
  if (res != 0) explode_bomb();
  if (n2 != 0) explode_bomb();
}

func4(int_32 n1, int_32 n2, int_32 n3) {
  int_32 mid = (n3 - n2) / 2 + n2;
  if (mid == n1) {
    return 0;
  } else if (mid < n1) {
    int_32 res = func4(n1, mid + 1, n3);
    return 1 + 2 * res;
  } else {
    int_32 res = func4(n1, n2, mid - 1);
    return res * 2;
  }
}
```

Firstly, it can be noted that the assembly code like `sub $0x8,%rsp`, which always comes first in a procedure, is used to align the address of the `call` instruction in the stack to multiple of 16. For example, the address of the section of `func4` in the stack begins with multiple of 16, and we assume it as `16n`. While the `call` instruction in the `func4` will store the address of next instruction into the stack first, and hence offset 8 bytes, we use `sub $0x8,%rsp` to offset 8 more bytes. As a result, the callee section in the stack will begin at `16n - 8 - 8`, which is also the multiple of 16. This satisfies the specification of `System V AMD64 ABI`.

Secondly, the following instructions are used to compute the mid point if it's negative:

```asm
  400fd2: 89 d0                 mov    %edx,%eax
  400fd4: 29 f0                 sub    %esi,%eax
  400fd6: 89 c1                 mov    %eax,%ecx
  400fd8: c1 e9 1f              shr    $0x1f,%ecx
  400fdb: 01 c8                 add    %ecx,%eax
  400fdd: d1 f8                 sar    $1,%eax
```

For example, if `a > b`, this means `(b - a)` is negative. As a routine in C, results of dividing negative numbers should be rounded towards zero. This is what instructions above do.

As the pseudocode above shown, the answer is `7 0`.

### Phase 5

The relevant logic for phase 5 is shown below:

```asm
0000000000401062 <phase_5>:
  401062: 53                    push   %rbx
  401063: 48 83 ec 20           sub    $0x20,%rsp
  401067: 48 89 fb              mov    %rdi,%rbx
  40106a: 64 48 8b 04 25 28 00  mov    %fs:0x28,%rax
  401071: 00 00
  401073: 48 89 44 24 18        mov    %rax,0x18(%rsp)
  401078: 31 c0                 xor    %eax,%eax
  40107a: e8 9c 02 00 00        call   40131b <string_length>
  40107f: 83 f8 06              cmp    $0x6,%eax
  401082: 74 4e                 je     4010d2 <phase_5+0x70>
  401084: e8 b1 03 00 00        call   40143a <explode_bomb>
  401089: eb 47                 jmp    4010d2 <phase_5+0x70>
  40108b: 0f b6 0c 03           movzbl (%rbx,%rax,1),%ecx
  40108f: 88 0c 24              mov    %cl,(%rsp)
  401092: 48 8b 14 24           mov    (%rsp),%rdx
  401096: 83 e2 0f              and    $0xf,%edx
  401099: 0f b6 92 b0 24 40 00  movzbl 0x4024b0(%rdx),%edx
  4010a0: 88 54 04 10           mov    %dl,0x10(%rsp,%rax,1)
  4010a4: 48 83 c0 01           add    $0x1,%rax
  4010a8: 48 83 f8 06           cmp    $0x6,%rax
  4010ac: 75 dd                 jne    40108b <phase_5+0x29>
  4010ae: c6 44 24 16 00        movb   $0x0,0x16(%rsp)
  4010b3: be 5e 24 40 00        mov    $0x40245e,%esi
  4010b8: 48 8d 7c 24 10        lea    0x10(%rsp),%rdi
  4010bd: e8 76 02 00 00        call   401338 <strings_not_equal>
  4010c2: 85 c0                 test   %eax,%eax
  4010c4: 74 13                 je     4010d9 <phase_5+0x77>
  4010c6: e8 6f 03 00 00        call   40143a <explode_bomb>
  4010cb: 0f 1f 44 00 00        nopl   0x0(%rax,%rax,1)
  4010d0: eb 07                 jmp    4010d9 <phase_5+0x77>
  4010d2: b8 00 00 00 00        mov    $0x0,%eax
  4010d7: eb b2                 jmp    40108b <phase_5+0x29>
  4010d9: 48 8b 44 24 18        mov    0x18(%rsp),%rax
  4010de: 64 48 33 04 25 28 00  xor    %fs:0x28,%rax
  4010e5: 00 00
  4010e7: 74 05                 je     4010ee <phase_5+0x8c>
  4010e9: e8 42 fa ff ff        call   400b30 <__stack_chk_fail@plt>
  4010ee: 48 83 c4 20           add    $0x20,%rsp
  4010f2: 5b                    pop    %rbx
  4010f3: c3                    ret
```

The corresponding logic in c-like pseudocode is shown below:

```c
char_8 *str1 = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?";
char_8 *str2 = "flyers";

phase_5(char *input) {
  int_32 res = string_length(input);
  if (res != 0x6) explode_bomb();
  char_8 arrs[7];
  for (int_32 i = 0; i < 0x6; ++i) {
    char_8 tmp = *(input + i);
    arrs[i] = *(str1 + (tmp & 0xf));
  }
  arrs[6] = 0;
  res = strings_not_equal(arrs, str2);
  if (res != 0) explode_bomb();
}
```

We can get specified strings by using `x/s 0x4024b0` and `x/s 0x40245e`. There's also a canary value stored in `0x18(%rsp)`, and we can compare its value later to look up if there's any buffer-overflow happens.

As the pseudocode shows, we can offset `str1` from 0 to 15, so the substring that can be chosen is `maduiersnfotvbyl`. We can refer to ASCII Table to choose proper character to index the character we want.

| Char | Value | Corresponding Char |
| :--: | :---: | :----------------: |
|  @   | 0x40  |         m          |
|  A   | 0x41  |         a          |
|  B   | 0x42  |         d          |
|  C   | 0x43  |         u          |
|  D   | 0x44  |         i          |
|  E   | 0x45  |         e          |
|  F   | 0x46  |         r          |
|  G   | 0x47  |         s          |
|  H   | 0x48  |         n          |
|  I   | 0x49  |         f          |
|  J   | 0x4a  |         o          |
|  K   | 0x4b  |         t          |
|  L   | 0x4c  |         v          |
|  M   | 0x4d  |         b          |
|  N   | 0x4e  |         y          |
|  O   | 0x4f  |         l          |

So the answer is shown below:

```txt
IONEFG
```

### Phase 6

The relevant logic for phase 6 is shown below:

```asm
00000000004010f4 <phase_6>:
  4010f4: 41 56                 push   %r14
  4010f6: 41 55                 push   %r13
  4010f8: 41 54                 push   %r12
  4010fa: 55                    push   %rbp
  4010fb: 53                    push   %rbx
  4010fc: 48 83 ec 50           sub    $0x50,%rsp
  401100: 49 89 e5              mov    %rsp,%r13
  401103: 48 89 e6              mov    %rsp,%rsi
  401106: e8 51 03 00 00        call   40145c <read_six_numbers>
  40110b: 49 89 e6              mov    %rsp,%r14
  40110e: 41 bc 00 00 00 00     mov    $0x0,%r12d
  401114: 4c 89 ed              mov    %r13,%rbp
  401117: 41 8b 45 00           mov    0x0(%r13),%eax
  40111b: 83 e8 01              sub    $0x1,%eax
  40111e: 83 f8 05              cmp    $0x5,%eax
  401121: 76 05                 jbe    401128 <phase_6+0x34>
  401123: e8 12 03 00 00        call   40143a <explode_bomb>
  401128: 41 83 c4 01           add    $0x1,%r12d
  40112c: 41 83 fc 06           cmp    $0x6,%r12d
  401130: 74 21                 je     401153 <phase_6+0x5f>
  401132: 44 89 e3              mov    %r12d,%ebx
  401135: 48 63 c3              movslq %ebx,%rax
  401138: 8b 04 84              mov    (%rsp,%rax,4),%eax
  40113b: 39 45 00              cmp    %eax,0x0(%rbp)
  40113e: 75 05                 jne    401145 <phase_6+0x51>
  401140: e8 f5 02 00 00        call   40143a <explode_bomb>
  401145: 83 c3 01              add    $0x1,%ebx
  401148: 83 fb 05              cmp    $0x5,%ebx
  40114b: 7e e8                 jle    401135 <phase_6+0x41>
  40114d: 49 83 c5 04           add    $0x4,%r13
  401151: eb c1                 jmp    401114 <phase_6+0x20>
  401153: 48 8d 74 24 18        lea    0x18(%rsp),%rsi
  401158: 4c 89 f0              mov    %r14,%rax
  40115b: b9 07 00 00 00        mov    $0x7,%ecx
  401160: 89 ca                 mov    %ecx,%edx
  401162: 2b 10                 sub    (%rax),%edx
  401164: 89 10                 mov    %edx,(%rax)
  401166: 48 83 c0 04           add    $0x4,%rax
  40116a: 48 39 f0              cmp    %rsi,%rax
  40116d: 75 f1                 jne    401160 <phase_6+0x6c>
  40116f: be 00 00 00 00        mov    $0x0,%esi
  401174: eb 21                 jmp    401197 <phase_6+0xa3>
  401176: 48 8b 52 08           mov    0x8(%rdx),%rdx
  40117a: 83 c0 01              add    $0x1,%eax
  40117d: 39 c8                 cmp    %ecx,%eax
  40117f: 75 f5                 jne    401176 <phase_6+0x82>
  401181: eb 05                 jmp    401188 <phase_6+0x94>
  401183: ba d0 32 60 00        mov    $0x6032d0,%edx
  401188: 48 89 54 74 20        mov    %rdx,0x20(%rsp,%rsi,2)
  40118d: 48 83 c6 04           add    $0x4,%rsi
  401191: 48 83 fe 18           cmp    $0x18,%rsi
  401195: 74 14                 je     4011ab <phase_6+0xb7>
  401197: 8b 0c 34              mov    (%rsp,%rsi,1),%ecx
  40119a: 83 f9 01              cmp    $0x1,%ecx
  40119d: 7e e4                 jle    401183 <phase_6+0x8f>
  40119f: b8 01 00 00 00        mov    $0x1,%eax
  4011a4: ba d0 32 60 00        mov    $0x6032d0,%edx
  4011a9: eb cb                 jmp    401176 <phase_6+0x82>
  4011ab: 48 8b 5c 24 20        mov    0x20(%rsp),%rbx
  4011b0: 48 8d 44 24 28        lea    0x28(%rsp),%rax
  4011b5: 48 8d 74 24 50        lea    0x50(%rsp),%rsi
  4011ba: 48 89 d9              mov    %rbx,%rcx
  4011bd: 48 8b 10              mov    (%rax),%rdx
  4011c0: 48 89 51 08           mov    %rdx,0x8(%rcx)
  4011c4: 48 83 c0 08           add    $0x8,%rax
  4011c8: 48 39 f0              cmp    %rsi,%rax
  4011cb: 74 05                 je     4011d2 <phase_6+0xde>
  4011cd: 48 89 d1              mov    %rdx,%rcx
  4011d0: eb eb                 jmp    4011bd <phase_6+0xc9>
  4011d2: 48 c7 42 08 00 00 00  movq   $0x0,0x8(%rdx)
  4011d9: 00
  4011da: bd 05 00 00 00        mov    $0x5,%ebp
  4011df: 48 8b 43 08           mov    0x8(%rbx),%rax
  4011e3: 8b 00                 mov    (%rax),%eax
  4011e5: 39 03                 cmp    %eax,(%rbx)
  4011e7: 7d 05                 jge    4011ee <phase_6+0xfa>
  4011e9: e8 4c 02 00 00        call   40143a <explode_bomb>
  4011ee: 48 8b 5b 08           mov    0x8(%rbx),%rbx
  4011f2: 83 ed 01              sub    $0x1,%ebp
  4011f5: 75 e8                 jne    4011df <phase_6+0xeb>
  4011f7: 48 83 c4 50           add    $0x50,%rsp
  4011fb: 5b                    pop    %rbx
  4011fc: 5d                    pop    %rbp
  4011fd: 41 5c                 pop    %r12
  4011ff: 41 5d                 pop    %r13
  401201: 41 5e                 pop    %r14
  401203: c3                    ret
```

The corresponding logic in c-like pseudocode is shown below:

```c
struct node {
  int32_t value;
  int32_t index;
  struct node *next;
}

struct node *list_head = 0x6032d0;

phase_6(char *input) {
  // section 1: read and validate input
  int32_t indices[6];
  read_six_numbers(input, indices);

  for (int32_t i = 0; i < 6; ++i) {
    int32_t normalized = indices[i] - 1;
    if ((uint32_t)normalized > 5)
      explode_bomb();

    for (int32_t j = i + 1; j < 6; ++j) {
      if (indices[j] == indices[i])
        explode_bomb();
    }
  }

  // section 2: transform indices
  for (int32_t *p = indices; p != indices + 6; ++p) {
    *p = 7 - *p;
  }

  // section 3: select nodes
  struct node *selected[6];
  for (int32_t i = 0; i < 6; ++i) {
    int32_t position = indices[i];
    struct node *current = list_head;

    if (position > 1) {
      for (int32_t step = 1; step != position; ++step) {
        current = current->next;
      }
    }
    selected[i] = current;
  }

  // section 4: relink nodes
  for (int32_t i = 0; i < 5; ++i) {
    selected[i]->next = selected[i + 1];
  }
  selected[5]->next = 0;

  // section 5: verify ordering
  for (int32_t i = 0; i < 5; ++i) {
    if (selected[i]->value < selected[i]->next->value)
      explode_bomb();
  }
}
```

The original assembly code contains some obscure logic, such as the loop incrementing i by 4 each time to more easily index the two arrays. I have hidden this logic to make the derived pseudocode easier to understand.

It's easy to find that there exists a node list, starting from address of 0x6032d0, and there are 6 nodes in the list. Since the assembly code using 8-bit offset to locate the pointer that points towards next node, we can assume that each node is 16 bytes long.
We can use `x/24xw 0x6032d0` to display it in `GDB` (Fortunately, these nodes are contiguous in memory.):

```txt
(gdb) x/24xw 0x6032d0
0x6032d0 <node1>:       0x0000014c      0x00000001      0x006032e0      0x00000000
0x6032e0 <node2>:       0x000000a8      0x00000002      0x006032f0      0x00000000
0x6032f0 <node3>:       0x0000039c      0x00000003      0x00603300      0x00000000
0x603300 <node4>:       0x000002b3      0x00000004      0x00603310      0x00000000
0x603310 <node5>:       0x000001dd      0x00000005      0x00603320      0x00000000
0x603320 <node6>:       0x000001bb      0x00000006      0x00000000      0x00000000
```

So it can be inferred that the node has the following structure:

```c
struct node {
  int32_t value;
  int32_t index;
  struct node *next;
}
```

As a result, the node list can be roughly displayed as:

```txt
(332,1) -> (168,2) -> (924,3) -> (691,4) -> (477,5) -> (443,6)
```

Section 1 indicates us that the input consists of 6 different numbers, ranging from **1** to 6. These numbers represent indices of a node list.
Section 2 transforms the indices, such as changing 1 to 6, 3 to 4, etc.
Section 3 stores pointers towards the nodes of the node list into `selected` array, corresponding to the indices specified by `indices`.
Section 4 reconnects nodes in the order given by the `selected` array.
Section 5 verifies nodes arranged in descending order of value.

We deduce the answer by proceeding in reverse order. To get the new list in which nodes arranged in descending order of value, which is as followed:

```txt
(924,3) -> (691,4) -> (477,5) -> (443,6) -> (332,1) -> (168,2)
```

We need the following array:

```txt
[3,4,5,6,1,2]
```

So, the answer is:

```txt
4,3,2,1,6,5
```

### Secret Phase

The relevant logic for secret phase is shown below:

```asm
0000000000401242 <secret_phase>:
  401242: 53                    push   %rbx
  401243: e8 56 02 00 00        call   40149e <read_line>
  401248: ba 0a 00 00 00        mov    $0xa,%edx
  40124d: be 00 00 00 00        mov    $0x0,%esi
  401252: 48 89 c7              mov    %rax,%rdi
  401255: e8 76 f9 ff ff        call   400bd0 <strtol@plt>
  40125a: 48 89 c3              mov    %rax,%rbx
  40125d: 8d 40 ff              lea    -0x1(%rax),%eax
  401260: 3d e8 03 00 00        cmp    $0x3e8,%eax
  401265: 76 05                 jbe    40126c <secret_phase+0x2a>
  401267: e8 ce 01 00 00        call   40143a <explode_bomb>
  40126c: 89 de                 mov    %ebx,%esi
  40126e: bf f0 30 60 00        mov    $0x6030f0,%edi
  401273: e8 8c ff ff ff        call   401204 <fun7>
  401278: 83 f8 02              cmp    $0x2,%eax
  40127b: 74 05                 je     401282 <secret_phase+0x40>
  40127d: e8 b8 01 00 00        call   40143a <explode_bomb>
  401282: bf 38 24 40 00        mov    $0x402438,%edi
  401287: e8 84 f8 ff ff        call   400b10 <puts@plt>
  40128c: e8 33 03 00 00        call   4015c4 <phase_defused>
  401291: 5b                    pop    %rbx
  401292: c3                    ret
  401293: 90                    nop
  401294: 90                    nop
  401295: 90                    nop
  401296: 90                    nop
  401297: 90                    nop
  401298: 90                    nop
  401299: 90                    nop
  40129a: 90                    nop
  40129b: 90                    nop
  40129c: 90                    nop
  40129d: 90                    nop
  40129e: 90                    nop
  40129f: 90                    nop
```

The corresponding logic in c-like pseudocode is shown below:

```c
struct node {
  int32_t value;
  struct node *left;
  struct node *right;
}
struct node *list_head = 0x6030f0;

secret_phase() {
  char *input = read_line();
  int32_t num = strtol(input, NULL, 10);
  if (((uint32_t)(num - 1)) > 1000) explode_bomb();
  int32_t num2 = fun7(list_head, num);
  if (num2 != 2) explode_bomb();
  puts("Wow! You've defused the secret stage!");
  phase_defused();
}

fun7(struct node* ptr, int32_t num) {
  if (ptr == NULL) return 0xffffffff;
  int32_t val = ptr->value;
  if (num == val) {
    return 0;
  } else if (val < num) {
    int32_t res = fun7(ptr->right, num);
    return res * 2 + 1;
  } else {
    int32_t res = fun7(ptr->left, num);
    return res * 2;
  }
}
```

By the following three instructions in assembly code, we can assume that the data structure is a binary tree:

```asm
  40120d: 8b 17                 mov    (%rdi),%edx     ;move 4-bit value
  401213: 48 8b 7f 08           mov    0x8(%rdi),%rdi  ;move 8-bit value
  401229: 48 8b 7f 10           mov    0x10(%rdi),%rdi ;move 8-bit value
```

The code reads an integer from the input first, and then finds it in a binary tree. Since it needs 2 as the answer, the number should be the value of `list_head->left->right`. Using `x/3xg 0x6030f0`, we can view the value of the node one by one:

```txt
(gdb) x/3xg 0x6030f0
0x6030f0 <n1>:  0x0000000000000024      0x0000000000603110
0x603100 <n1+16>:       0x0000000000603130
(gdb) x/3xg 0x603110
0x603110 <n21>: 0x0000000000000008      0x0000000000603190
0x603120 <n21+16>:      0x0000000000603150
(gdb) x/3xg 0x603150
0x603150 <n32>: 0x0000000000000016      0x0000000000603270
0x603160 <n32+16>:      0x0000000000603230
```

So the answer is 22.

To trigger the secret phase, we need to dive into the `phase_defused`. Firstly, it makes sure that six phases have been solved. Next, it reads two numbers and a string from the global region. Using `x/s 0x603870`, we see the string `7 0`, the answer of phase 4. And, by `x/s 0x402622`, we see `DrEvil` as the expected string. So, we need to append `DrEvil` to the end of the answer of phase 4, as `7 0 DrEvil`, to trigger the secret phase.

The final output should be like this:

```txt
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Phase 1 defused. How about the next one?
That's number 2.  Keep going!
Halfway there!
So you got that one.  Try this one.
Good work!  On to the next...
Curses, you've found the secret phase!
But finding it and solving it are quite different...
Wow! You've defused the secret stage!
Congratulations! You've defused the bomb!
```

Till now, we have completed this lab. Congratulations!
