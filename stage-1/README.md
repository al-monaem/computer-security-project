# Stage 1 — Classic stack buffer overflow with injected shellcode (x86-64)

The baseline attack the whole project is defined against: **code injection on an
executable stack.** A crafted overflow overwrites a function's saved return
address so that, when the function returns, the CPU jumps into attacker-supplied
machine code sitting on the stack and runs it.

This stage introduces the program the whole project is built on
(`review_intake.c`), compiled here with the stack left **executable**
(`-z execstack`). That is deliberate: Stages 1 → 2 → 3 all attack this *same*
program, and the *only* thing that changes between them is the defense.

| Stage | NX (stack) | Technique | Result |
|-------|------------|-----------|--------|
| **1 (here)** | **OFF** | inject shellcode on the stack, return into it | shell |
| 2 | ON | *same* exploit, re-run | SIGSEGV |
| 3 | ON | ROP / ret2libc (reuse existing code) | shell |

> ⚠️ **Educational use only.** `review_intake.c` is intentionally vulnerable.
> Build and run it only inside your own Linux VM (e.g. SEED Labs). Do not deploy
> it.

---

## 1. The idea in one paragraph

`submit_review()` declares a 64-byte `comment` buffer, then `read()`s up to 256
bytes into it with no bounds check. By writing past the end of `comment` we reach
the saved return address higher up the stack and replace it with the address of
the buffer itself — where we have placed real `execve("/bin/sh")` shellcode. When
`submit_review()` returns, execution lands in our bytes on the stack and runs
them. Because this stage is compiled with an **executable stack**
(`-z execstack`), the injected code is allowed to execute, and we get a shell.

Payload layout:

```
[ execve("/bin/sh") shellcode ][ 0x90 padding ... ][ saved-RA -> comment buffer ]
  ~44 bytes                      up to offset 88      8 bytes (little-endian)
```

---

## 2. Hitting the exact stack address (why this uses gdb)

The hard part of the classic attack is knowing the buffer's **exact runtime
address**. It shifts with the process environment and `argv[0]`, so an address
measured under one launch is stale under another — with ASLR off each launch is
deterministic, but different launchers lay the stack out differently.

**`exploit_stack.py` sidesteps the guessing entirely**: it runs the target
*under gdb* (`gdb.debug`), breaks inside `submit_review`, and
reads the **live** address of `comment` and the **live** saved-return-address
slot from the very process it then exploits. Nothing is hardcoded and no address
is predicted — it is measured in-place, so the jump is exact.

The overflow offset (distance from `comment` to the saved return address) is
**88**; the script derives it from gdb's frame info each run, so it survives
recompiles.

---

## 3. Files

| File | What it is |
|------|------------|
| `review_intake.c` | The vulnerable program — the project's baseline, reused unchanged by Stages 2–3; built NX-off here. |
| `Makefile` | Builds x86-64 with canary off, PIE off, **executable stack**; `make checksec` prints the protections. |
| `exploit_stack.py` | Injects `execve("/bin/sh")` and returns into the stack buffer (live gdb-measured address) → interactive shell. |
| `.gitignore` | Keeps the compiled binary and corefiles out of git. |

---

## 4. Setup

```bash
# 64-bit toolchain is standard on a 64-bit Ubuntu/SEED VM (build-essential).
# pwntools: either the system package or a venv.
sudo apt install -y python3-pwntools        # simplest on SEED/Kali
# or:  python3 -m venv ropenv && ropenv/bin/pip install pwntools
```

---

## 5. Build and run

```bash
cd stage-1
make                 # builds the target; prints the NX/canary/PIE summary
python3 exploit_stack.py
```

Confirm the stack really is executable (this is what makes Stage 1 work):

```bash
make checksec
# NX (stack)  : DISABLED (stack executable)  <-- enables Stage 1 shellcode injection
```

`exploit_stack.py` launches the target under gdb, so it needs a terminal for the
gdb window — run it from the SEED **desktop terminal**, or inside `tmux`, or set
`context.terminal = ['gnome-terminal', '--']` near the top of the script.

Expected: it prints the measured offset and the live `comment` address, sends the
payload, and drops you at a `$` prompt. Try `id`, `uname -a`, `cat /etc/hostname`.

---

## 6. How this connects to the next stages

Rebuild this **same source** with NX on (via `-z noexecstack`) and re-run
`exploit_stack.py`: the jump into the stack now faults with **SIGSEGV** instead
of giving a shell — that is Stage 2, and it needs no new code, only the flag
change. To get a shell again under NX you must stop injecting code and start
reusing it: that is Stage 3's ROP / ret2libc chain — the same overflow, aimed at
existing executable code instead of at bytes on the stack.

---

## 7. Environment notes

- The offset and buffer address are **measured at runtime** (via gdb), so you do
  not need to edit anything after recompiling.
- Verified on a 64-bit SEED Ubuntu 20.04 VM (glibc 2.31) with the stack mapped
  `rwxp`. Everything here is meant to run inside an isolated Linux VM.
