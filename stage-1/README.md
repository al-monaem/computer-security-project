# Stage 1 — Classic stack buffer overflow with injected shellcode (x86-64)

A minimal vulnerable program that reads a file into an oversized stack buffer
with no bounds checking. A crafted payload overflows the buffer, overwrites the
saved return address, and redirects execution into benign machine code placed on
the stack. This is the baseline attack — **code injection on an executable
stack** — that later stages (NX, ROP, ASLR) are defined against.

> ⚠️ **Educational use only.** The target program is intentionally vulnerable.
> Build and run it only on your own machine (e.g. the SEED Labs VM), for
> learning. Do not deploy it.

---

## 1. The idea in one paragraph

`vulnerable()` declares a 64-byte `buffer` and then `memcpy`s up to 256 bytes of
attacker-controlled file data into it with no length check. By writing past the
end of `buffer` we reach the saved return address further up the stack. We
replace that address with the address of our own bytes — a tiny instruction
sequence we placed at the start of the buffer. When `vulnerable()` returns, the
CPU jumps into our bytes and executes them. The stack is compiled executable
(`-z execstack`), so the injected code is allowed to run.

The injected code here is deliberately harmless: `mov eax, 42; ret`. Seeing
`rax = 0x2a` (42) after the return proves our bytes executed.

---

## 2. Files

| File                   | What it is                                                               |
|------------------------|--------------------------------------------------------------------------|
| `shellcode_overflow.c` | The intentionally vulnerable program (unbounded `memcpy` into a buffer). |
| `generate_payload.py`  | Builds `shellcode_payload.bin`: injected code + padding + return address.|
| `Makefile`             | Builds the target with canary off, PIE off, executable stack.            |
| `.gitignore`           | Keeps the compiled binary and generated payload out of git.              |

The payload layout (`generate_payload.py`):

```
[ mov eax,42; ret ][ 0x90 padding ... ][ saved-RA -> buffer_address ]
 6 bytes            up to offset (88)    8 bytes (little-endian)
= 96 bytes total
```

`offset = 88` is the distance from the start of `buffer` to the saved return
address; `buffer_address = 0x7fffffffdf30` is where the buffer lives in the
recorded GDB run.

---

## 3. Build and generate the payload

Run these inside your Ubuntu SEED Labs VM, in your Stage 1 directory:

```bash
cd ~/stack-overflow-project/stage1

gcc -g -O0 -fno-stack-protector \
    -fno-omit-frame-pointer -no-pie \
    -z execstack \
    -o shellcode_overflow shellcode_overflow.c

python3 generate_payload.py
```

Or, equivalently, with the Makefile:

```bash
make
make payload
```

Check that the stack is executable:

```bash
readelf -W -l shellcode_overflow | grep GNU_STACK   # or: make checkstack
```

Expected permission flags: **RWE**.

---

## 4. GDB demonstration

Start GDB:

```bash
gdb ./shellcode_overflow
```

Inside GDB:

```gdb
break *0x401316
run shellcode_payload.bin
```

Inspect the saved return address:

```gdb
x/gx $rsp
```

In the recorded run, the expected result was:

```
0x7fffffffdf88: 0x00007fffffffdf30
```

Inspect the injected instructions:

```gdb
x/2i 0x7fffffffdf30
```

Then execute the return and inspect the registers:

```gdb
si
info registers rip rax
```

In the original GDB session, PEDA stepped through the injected `mov`
instruction, producing:

```
rip  0x7fffffffdf35
rax  0x2a
```

`0x2a` is 42 in decimal, confirming execution of the injected code. Depending on
GDB's stepping behavior, you may need another `si` before inspecting the
registers.

---

## 5. Environment-specific values

The addresses below are from the original build and debugger session. **Verify
them if you recompile or change the environment** — the stack address in
particular will differ, and the payload's `buffer_address` must be updated to
match.

| Value             | Where it comes from                        |
|-------------------|--------------------------------------------|
| `0x401316`        | Breakpoint: the `ret` in `vulnerable()`.   |
| `0x7fffffffdf30`  | Address of `buffer` (payload target).      |
| `0x7fffffffdf88`  | Location of the saved return address.      |
| `offset = 88`     | Distance from `buffer` to the saved RA.    |

If the stack address changes, re-read `buffer`'s address in GDB and update
`buffer_address` in `generate_payload.py`.
