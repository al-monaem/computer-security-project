# ROP vs. NX — a 32-bit x86 demonstration

A small, realistic command-line application (a product-review intake tool) that
contains one classic stack buffer overflow, plus a working **Return-Oriented
Programming (ROP)** exploit that defeats the **NX (No-eXecute)** defense and
spawns a shell.

> ⚠️ **Educational use only.** The target program is intentionally vulnerable.
> Build and run it only on your own machine, for learning. Do not deploy it.

---

## 1. The idea in one paragraph

Modern CPUs can mark memory pages **non-executable (NX / DEP)**. The stack is
marked that way, so the old attack — "copy shellcode onto the stack and jump to
it" — fails: the CPU refuses to execute instructions fetched from the stack.
**ROP** sidesteps this. Instead of injecting *new* code, the attacker reuses
*existing* executable code — small snippets ending in `ret`, called **gadgets**,
and whole library functions like `system()`. The overflow is used only to write
a sequence of addresses onto the stack; each `ret` jumps to the next piece of
borrowed code. Nothing on the stack is ever executed as code, so NX never fires.

This demo shows both halves:

- `nx_shellcode_fails.py` — the stack-shellcode attack **crashing** because of NX.
- `exploit.py` — a multi-gadget ROP chain that **succeeds**: it hand-assembles
  the `execve("/bin/sh", NULL, NULL)` syscall from four separate gadgets.

---

## 2. Files

| File                     | What it is                                                        |
|--------------------------|-------------------------------------------------------------------|
| `review_intake.c`        | The realistic app with one planted stack buffer overflow.         |
| `Makefile`               | Builds it 32-bit with **NX on**, canary off, PIE off. `make checksec`. |
| `find_offset.py`         | Auto-computes the overflow offset with a cyclic pattern.          |
| `nx_shellcode_fails.py`  | Shows stack shellcode dying under NX (the motivation for ROP).    |
| `exploit.py`             | Multi-gadget ROP chain: builds the execve syscall and pops a shell.|
| `exploit_ret2libc.py`    | The simple cousin: one jump into `system("/bin/sh")` (compare).   |
| `exploit_hardcoded.py`   | Frozen addresses — works with ASLR off, **crashes** with ASLR on. |
| `exploit_aslr.py`        | Two-stage: leaks `puts` to defeat ASLR, then the execve chain.    |
| `exploit_mprotect.py`    | ROP calls `mprotect` to make a page RWX, then runs shellcode.     |
| `README.md`              | This document.                                                    |

---

## 3. Setup

### 3.1 Install the 32-bit toolchain (one time)

```bash
sudo apt install -y gcc-multilib libc6-dev-i386
```

### 3.2 Python tooling (pwntools + ROPgadget)

Already installed into a virtual environment during setup. Activate it:

```bash
source "<scratchpad>/ropenv/bin/activate"   # exact path printed by the assistant
```

(or just call that venv's `python` directly).

### 3.3 Build

```bash
make
```

`make` prints a security summary. You want to see:

```
  Arch        : 32-bit
  NX (stack)  : ENABLED  (stack non-executable)  <-- the defense we defeat
  Stack canary: ABSENT (overflow not caught)
  PIE         : NO (fixed load address)
```

---

## 4. Running the demo

For slides/grading, run everything inside an ASLR-disabled shell so addresses
are identical every time (needs no root):

```bash
setarch "$(uname -m)" -R bash      # or:  make aslr-off
```

Then:

```bash
# (a) See the real application behave normally:
./review_intake

# (b) Watch NX kill the classic stack-shellcode attack:
python3 nx_shellcode_fails.py
#     -> target crashes with SIGSEGV. That is NX doing its job.

# (c) Watch ROP defeat NX and pop a shell:
python3 exploit.py
#     -> you get a shell:  $ id ; uname -m ; cat /etc/hostname
```

---

## 5. How the vulnerability works

In `submit_review()`:

```c
char comment[64];                 /* 64-byte stack buffer            */
...
int n = read(STDIN_FILENO, comment, 256);   /* reads up to 256 bytes */
```

The buffer holds 64 bytes but `read()` accepts up to 256. Bytes 65+ run off the
end of the buffer and, going up the stack, overwrite the **saved frame pointer**
and then the **saved return address**. When `submit_review()` executes its
`ret`, the CPU pops *our* value into `EIP` and jumps wherever we chose.

`read()` (rather than `scanf`/`fgets`) matters: it copies raw bytes of **any**
value, including NUL and whitespace, so we can place arbitrary 4-byte addresses.

### Stack layout at the overflow

```
    lower addresses
    +------------------------+  <- &comment
    |  comment[64]           |   our input starts here
    +------------------------+
    |  saved EBP (4 bytes)   |   overwritten by padding
    +------------------------+
    |  saved return address  |   <-- we overwrite this (offset = OFFSET bytes)
    +------------------------+
    |  ... (our ROP chain)   |   addresses we place become the program flow
    +------------------------+
    higher addresses
```

---

## 6. Finding the offset

The distance from the start of `comment[]` to the saved return address depends
on how the compiler laid out the frame, so we measure it instead of guessing:

```bash
python3 find_offset.py
# ================================================
#   Overflow offset to return address: 84
#   --> set  OFFSET = 84  in exploit.py
# ================================================
```

It sends a cyclic (De Bruijn) pattern, lets the program crash on the garbage
return address, reads the faulting `EIP` from the core dump, and looks up which
4-byte window of the pattern that was — which is exactly the offset.

For this build the offset is **84** (64 bytes of buffer + 16 bytes of other
locals/alignment + 4 bytes of saved EBP). `exploit.py` already uses it.

---

## 7. How the ROP exploit works (a real gadget chain)

NX means we cannot run our own code, so we reuse code that is already there.
But instead of a single jump into a convenience function like `system()`, this
exploit builds the `execve("/bin/sh", NULL, NULL)` **system call by hand** out
of four separate gadgets. A *gadget* is a tiny pre-existing instruction
sequence ending in `ret`; each `ret` pops the next gadget's address off our
stack, so the gadgets run one after another — that chaining is what ROP is.

On 32-bit Linux, the `execve` syscall is made by loading registers and
executing `int 0x80`:

```
  eax = 0x0b        number of the execve syscall
  ebx = &"/bin/sh"  path to run
  ecx = 0           argv = NULL
  edx = 0           envp = NULL
  int 0x80          hand control to the kernel
```

We set each of those from its own gadget. The chain laid on the stack:

```
  [ 'A' * 84                  ]  padding up to the saved return address
  [ &(pop ecx ; pop edx ; ret)]  gadget 1  --------------------+
  [ 0x00000000                ]    value popped into ecx  (argv = NULL)
  [ 0x00000000                ]    value popped into edx  (envp = NULL)
  [ &(pop ebx ; ret)          ]  gadget 2  --------------------+
  [ &"/bin/sh"                ]    value popped into ebx  (the path)
  [ &(pop eax ; ret)          ]  gadget 3  --------------------+
  [ 0x0000000b                ]    value popped into eax  (execve = 11)
  [ &(int 0x80)               ]  gadget 4  -> performs the syscall
```

Step by step, each `ret` drives the next gadget:

1. `submit_review()` does `ret`, popping the address of **gadget 1**
   (`pop ecx ; pop edx ; ret`) into `EIP`. That gadget pops our two zeros into
   `ecx` and `edx`, then `ret`s.
2. The `ret` pops **gadget 2** (`pop ebx ; ret`), which loads `ebx` with the
   address of the string `"/bin/sh"` (that string already lives inside libc).
3. Its `ret` pops **gadget 3** (`pop eax ; ret`), which loads `eax = 11`, the
   `execve` syscall number.
4. Its `ret` pops **gadget 4** (`int 0x80`). The registers now describe
   `execve("/bin/sh", NULL, NULL)`, so the kernel replaces the process with a
   shell.

**Why NX cannot stop this:** at no point do we execute bytes located on the
stack. The stack holds only *addresses and data*. Every instruction that runs
lives inside `libc`, which is legitimately executable. NX governs *where* code
may run, and a ROP chain never puts code where NX would object — it only
rearranges which existing code runs, and in what order.

You can list the kinds of gadgets available in any binary yourself:

```bash
ROPgadget --binary /usr/lib32/libc.so.6 --only "pop|ret" | grep ': pop e'
ROPgadget --binary /usr/lib32/libc.so.6 | grep ': int 0x80'
```

> **ret2libc vs. ROP chain.** Calling libc's `system("/bin/sh")` directly (one
> function, one jump) is the simpler cousin called *return-to-libc*. The chain
> above is "full" ROP: no single function does the work — four independent
> gadgets cooperate to set up and issue a raw syscall. Both defeat NX the same
> way (reusing executable code), but the chain shows the technique that scales
> to cases where no convenient function exists.

---

## 8. Expected output (sample)

```
$ python3 exploit.py
[+] Starting local process './review_intake': pid 603837
[*] ROP chain gadgets (runtime addresses):
[*]   [1] pop ecx ; pop edx ; ret   @ 0xf7da046b   (ecx=0, edx=0)
[*]   [2] pop ebx ; ret             @ 0xf7d886cc   (ebx=&/bin/sh)
[*]   [3] pop eax ; ret             @ 0xf7e94211   (eax=11 execve)
[*]   [4] int 0x80                  @ 0xf7d9e59c   (syscall)
[*]       "/bin/sh" string          @ 0xf7f2e167
[+] Chain delivered. Four gadgets assembled execve('/bin/sh',0,0).
[*] Switching to interactive mode
Thanks! Recorded 1094795585/5 for "demo-widget".
Comment on file: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
$ id
uid=1001(almonaemkhan) gid=1001(almonaemkhan) groups=1001(almonaemkhan),4(adm),24(cdrom),27(sudo)...
$ exit
```

Each of the four gadget addresses above is a separate borrowed instruction
fragment; the shell appears only after all four have run in sequence.

(The libc addresses will differ on your machine; with ASLR off they stay the
same across runs. The garbage rating `1094795585` is just our padding `AAAA`
being read as an integer — harmless, and a nice visual cue that the overflow
has begun.)

---

## 9. How each defense would have stopped this

This demo deliberately leaves everything on *except* the things that would make
the lesson impossible to see. In production you would have all of these:

| Defense | What it is | How it breaks this exploit |
|---|---|---|
| **NX / DEP** *(on here)* | Stack is non-executable. | Stops stack shellcode — which is *why* we needed ROP in the first place. |
| **Stack canary** *(off here)* | A random value placed before the return address, checked on return. | The overflow would smash the canary; the check aborts the program before `ret`, so we never control `EIP`. |
| **ASLR** *(off here)* | Randomizes libc's load address each run. | We could not hard-code `&system` / `&"/bin/sh"`; we'd first need an info-leak to discover libc's base. |
| **PIE** *(off here)* | Randomizes the program image too. | Gadgets inside the binary would also move, removing our fixed-address gadget. |
| **Fortify / safe APIs** | `read(buf, sizeof buf)`, `fgets`, bounds checks. | The overflow never happens — the real fix. |

### The real fix

```c
char comment[64];
int n = read(STDIN_FILENO, comment, sizeof(comment) - 1);  /* bound by the buffer */
if (n > 0) comment[n] = '\0';
```

Defenses like NX, canaries, ASLR, and PIE raise the cost of exploitation, but
the vulnerability is the unbounded write. Fixing the bug is what actually
removes it.

---

## 10. Cleanup

```bash
make clean
```
