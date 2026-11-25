# Computer Security Project — From Stack Smashing to Code Reuse

A reproducible teaching demonstration that follows one vulnerability class, the
stack buffer overflow, through its history: the classic overflow that injects
and runs machine code, the NX (No-eXecute / DEP / W^X) defense that stops it,
and the code-reuse techniques attackers used to get around NX (return-to-libc
and full Return-Oriented Programming), including an ASLR bypass via an
information leak and an `mprotect` chain.

The whole project attacks **one** deliberately vulnerable program
(`review_intake.c`). Across the stages the *only* thing that changes is the
defense configuration and the exploit technique — so each result isolates the
effect of a single mitigation.

> **Educational use only.** The programs here are intentionally vulnerable and
> run with harmless payloads inside an isolated VM. Do not deploy them, and test
> only in authorized environments.

## Repository layout

| Folder | Contents | Status |
|--------|----------|--------|
| `stage-1/` | Classic stack overflow with injected shellcode (x86-64, executable stack). | **present** |
| `stage-2/` | NX enabled; the Stage 1 exploit is re-run and fails, then return-to-libc recovers execution. | design only (see report) |
| `stage-3/` | ROP suite (x86-32): defeats NX by reusing existing code, plus an ASLR leak and an `mprotect` chain. | **present** |
| `latex/` | Two-column conference-paper write-up of the whole project. | **present** |

Stage 2 is not yet a separate folder: it is the *same* x86-64 source rebuilt
with one flag flipped (`-z execstack` → `-z noexecstack`), re-running the Stage 1
exploit to show it fault under NX. Its design is written up in the report
(`latex/report.tex`); the captured demo evidence is still being finalized.

## The three stages at a glance

| Stage | Arch | NX (stack) | Technique | Result |
|-------|------|------------|-----------|--------|
| **1** | x86-64 | **OFF** | Inject shellcode on the stack, return into it | shell |
| **2** | x86-64 | ON | *Same* exploit re-run → faults; then return-to-libc into `system("/bin/sh")` | SIGSEGV, then shell |
| **3** | x86-32 | ON | ROP / ret2libc (reuse existing code); + ASLR leak; + `mprotect` | shell despite NX and ASLR |

> **Why two architectures?** Stages 1–2 use an x86-64 build. The Stage 3 ROP
> prototype is 32-bit (x86-32) so that syscall arguments are passed on the
> stack, which keeps the gadget layout simple to teach. As a result the measured
> overflow offset differs by stage — **88** bytes on the x86-64 build, **84** on
> the x86-32 build — because each compiler reserves different frame space beyond
> the 64-byte buffer. Both are measured at runtime, not assumed.

## Stage 1 — Classic injection (x86-64)

The baseline attack the whole project is defined against: code injection on an
executable stack. The overflow overwrites the saved return address with the
address of the `comment` buffer itself, where `execve("/bin/sh")` shellcode has
been placed; when `submit_review()` returns, execution lands in those bytes and
runs them. `exploit_stack.py` measures the live buffer address and the saved-RA
slot under gdb, so nothing is hardcoded.

See **[`stage-1/README.md`](stage-1/README.md)** for the full write-up.

## Stage 3 — ROP vs. NX (x86-32)

A small, realistic vulnerable program plus a family of exploit scripts that each
target it under a specific defense configuration. The binary is built with NX
on, stack canary off, and PIE off, so the overflow is reachable and addresses
are deterministic for teaching.

| File | What it does |
|------|--------------|
| `review_intake.c` | The shared vulnerable program (64-byte buffer, 256-byte `read`), built here 32-bit with NX on. |
| `Makefile` | Builds 32-bit with NX on, canary off, PIE off; `make checksec` prints the protections. |
| `find_offset.py` | Finds the overflow offset (84) with a cyclic pattern. |
| `nx_shellcode_fails.py` | Shows stack shellcode dying under NX (SIGSEGV) — the motivation for ROP. |
| `exploit.py` | Four-gadget ROP chain that hand-assembles `execve("/bin/sh", NULL, NULL)`. |
| `exploit_ret2libc.py` | The simpler return-to-libc variant: one jump into `system("/bin/sh")`. |
| `exploit_hardcoded.py` | Frozen addresses: works with ASLR off, crashes with ASLR on (the bait). |
| `exploit_aslr.py` | Two-stage: leaks `puts` to defeat ASLR, then runs the ROP chain. |
| `exploit_mprotect.py` | ROP calls `mprotect` to make a page executable, then runs shellcode. |

See **[`stage-3/README.md`](stage-3/README.md)** for the full write-up (stack
diagrams, the gadget chain, and how each defense would stop the attack).

## Report

`latex/report.tex` builds a two-column conference-paper rendering of the whole
project; a pre-built `latex/report.pdf` is checked in. An interim PDF export,
`Fall2026_Interim Project Report_From Stack Smashsing to Code Reuse.pdf`, sits at
the repository root. See **[`latex/README.md`](latex/README.md)** to rebuild it.

## Setup (quick reference)

```bash
# Stage 1 (x86-64): standard toolchain on a 64-bit Ubuntu/SEED VM.
sudo apt install -y python3-pwntools        # or a venv with pwntools

# Stage 3 (x86-32): add the 32-bit toolchain.
sudo apt install -y gcc-multilib libc6-dev-i386
python3 -m venv ropenv                       # use a path without spaces
ropenv/bin/pip install pwntools ROPgadget
```

Each stage's README has the exact build-and-run steps. Everything is meant to
run inside an isolated Linux VM (e.g. SEED Labs).
