# Computer Security Project — From Stack Smashing to Code Reuse

A reproducible teaching demonstration that follows one vulnerability class, the
stack buffer overflow, through its history: the classic overflow that injects
and runs machine code, the NX (No-eXecute / DEP / W^X) defense that stops it,
and the code-reuse techniques attackers used to get around NX (return-to-libc
and full Return-Oriented Programming), including an ASLR bypass via an
information leak and an `mprotect` chain.

> **Educational use only.** The programs here are intentionally vulnerable and
> run with harmless payloads inside an isolated VM. Do not deploy them, and test
> only in authorized environments.

## Repository layout

| Folder | Contents | Status |
|--------|----------|--------|
| `stage-1/` | Classic stack overflow with injected shellcode (x86-64, executable stack). | *coming* |
| `stage-2/` | NX enabled; the Stage 1 exploit is re-run and fails. | *coming* |
| `stage-3/` | ROP prototype (x86-32): defeats NX by reusing existing code. | **here** |

## Stage 3 — ROP vs. NX (x86-32)

A small, realistic vulnerable program plus a family of exploit scripts that each
target it under a specific defense configuration. The binary is built with NX
on, stack canary off, and PIE off, so the overflow is reachable and addresses
are deterministic for teaching.

### Files

| File | What it does |
|------|--------------|
| `review_intake.c` | Realistic review-intake CLI with a planted stack buffer overflow (64-byte buffer, 256-byte `read`). |
| `Makefile` | Builds 32-bit with NX on, canary off, PIE off; `make checksec` prints the protections. |
| `find_offset.py` | Finds the overflow offset (84) with a cyclic pattern. |
| `nx_shellcode_fails.py` | Shows stack shellcode dying under NX (SIGSEGV) — the motivation for ROP. |
| `exploit.py` | Four-gadget ROP chain that hand-assembles `execve("/bin/sh", NULL, NULL)`. |
| `exploit_ret2libc.py` | The simpler return-to-libc variant: one jump into `system("/bin/sh")`. |
| `exploit_hardcoded.py` | Frozen addresses: works with ASLR off, crashes with ASLR on (the bait). |
| `exploit_aslr.py` | Two-stage: leaks `puts` to defeat ASLR, then runs the ROP chain. |
| `exploit_mprotect.py` | ROP calls `mprotect` to make a page executable, then runs shellcode. |

### Setup

```bash
# 32-bit toolchain (Debian/Ubuntu)
sudo apt install -y gcc-multilib libc6-dev-i386

# Python tooling in a venv (use a path without spaces)
python3 -m venv ropenv
ropenv/bin/pip install pwntools ROPgadget
```

### Build and run

```bash
cd stage-3
make                                   # builds the target; prints NX/canary/PIE summary
source ../ropenv/bin/activate

# For stable addresses while explaining, run under ASLR off:
setarch "$(uname -m)" -R python3 exploit.py        # pops a shell via ROP

python3 nx_shellcode_fails.py          # SIGSEGV: NX blocks stack shellcode
python3 exploit_aslr.py                # ASLR ON: leak + ROP chain -> shell
python3 exploit_mprotect.py            # mprotect -> shellcode
```

See `stage-3/README.md` for the full write-up (stack diagrams, the gadget chain,
and how each defense would stop the attack).
