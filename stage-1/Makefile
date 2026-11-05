# Stage 1 — classic stack buffer overflow with injected shellcode (x86-64)
#
# Built deliberately insecure for the demonstration:
#   -fno-stack-protector   : no stack canary
#   -fno-omit-frame-pointer : keep the frame pointer for predictable layout
#   -no-pie                : fixed load address
#   -z execstack           : executable stack (so injected code can run)

CC      := gcc
CFLAGS  := -g -O0 -fno-stack-protector -fno-omit-frame-pointer -no-pie
LDFLAGS := -z execstack
TARGET  := shellcode_overflow

.PHONY: all payload checkstack clean

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

payload: generate_payload.py
	python3 generate_payload.py

# Confirm the stack segment is writable+executable (expect flags RWE).
checkstack: $(TARGET)
	readelf -W -l $(TARGET) | grep GNU_STACK

clean:
	rm -f $(TARGET) shellcode_payload.bin
