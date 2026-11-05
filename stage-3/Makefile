# ============================================================================
#  Makefile -- 32-bit x86 ROP / NX demonstration
# ============================================================================
#
#  Build flags are chosen to create the standard "hardened everywhere EXCEPT
#  the bug" lab scenario:
#
#    -m32                 build 32-bit x86 (i386), the classic ROP target
#    -fno-stack-protector REMOVE the stack canary, so the overflow is not
#                         caught before the function returns
#    -z noexecstack       NX ON: the stack is marked non-executable.
#                         (This is also the linker default now; we set it
#                          explicitly so the intent is obvious.)
#    -no-pie -fno-pie     disable PIE, so the binary loads at fixed addresses
#                         (keeps the demo deterministic)
#    -g                   debug symbols, nice for gdb / objdump
#
#  NX being ON is the whole point: shellcode placed on the stack cannot run,
#  which is exactly why the exploit must use ROP / return-to-libc instead.
# ============================================================================

CC      := gcc
TARGET  := review_intake
SRC     := review_intake.c

CFLAGS  := -m32 -g -O0 -fno-stack-protector -no-pie -fno-pie \
           -Wno-deprecated-declarations
LDFLAGS := -m32 -no-pie -z noexecstack

.PHONY: all clean checksec run aslr-off help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo
	@echo "Built '$(TARGET)' (32-bit). Security properties:"
	@$(MAKE) --no-print-directory checksec

# Show the relevant binary protections using tools everyone has.
checksec: $(TARGET)
	@echo "---------------------------------------------"
	@printf "  Arch        : "; file $(TARGET) | grep -o '32-bit' | head -1
	@printf "  NX (stack)  : "; \
	  if readelf -l $(TARGET) | grep -q 'GNU_STACK.*RWE'; then \
	    echo "DISABLED (stack executable)"; \
	  else echo "ENABLED  (stack non-executable)  <-- the defense we defeat"; fi
	@printf "  Stack canary: "; \
	  if readelf -s $(TARGET) 2>/dev/null | grep -q '__stack_chk_fail'; then \
	    echo "present"; else echo "ABSENT (overflow not caught)"; fi
	@printf "  PIE         : "; \
	  if readelf -h $(TARGET) | grep -q 'Type:.*DYN'; then \
	    echo "yes (ASLR on code)"; else echo "NO (fixed load address)"; fi
	@echo "---------------------------------------------"

# Run the program normally so you can see the real app behavior.
run: $(TARGET)
	./$(TARGET)

# Launch a shell with ASLR disabled for this process tree, so libc sits at
# a fixed address and the demo is 100% reproducible. (Needs no root.)
# Inside the new shell, run:  python3 exploit.py
aslr-off:
	@echo "Starting a shell with ASLR disabled (type 'exit' to leave)..."
	setarch $$(uname -m) -R bash

clean:
	rm -f $(TARGET)

help:
	@echo "make            build the vulnerable binary (NX on, no canary, no PIE)"
	@echo "make checksec   show the binary's security properties"
	@echo "make run        run the app normally"
	@echo "make aslr-off   open a sub-shell with ASLR disabled (for a stable demo)"
	@echo "make clean      remove the binary"
