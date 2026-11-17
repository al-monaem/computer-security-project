# ============================================================================
#  Makefile -- Stage 1 variant: x86-64 shellcode injection (NX OFF)
# ============================================================================
#
#  This is the baseline build. The stack is deliberately left EXECUTABLE, so
#  shellcode placed on the stack is allowed to run. Everything else is kept
#  deterministic so the demo is reproducible:
#
#    -fno-stack-protector  REMOVE the stack canary (overflow not caught)
#    -no-pie -fno-pie      fixed load address (no PIE/code ASLR)
#    -z execstack          NX OFF: stack is Writable AND eXecutable (RWE)
#    -g -O0                debug symbols, no optimization (predictable layout)
#
#  The executable stack (-z execstack) is the whole point of Stage 1: it is
#  what lets injected machine code run. Stages 2 and 3 rebuild the SAME source
#  with -z noexecstack (NX ON) instead -- that one flip is what forces the
#  switch to ROP in Stage 3.
# ============================================================================

CC      := gcc
TARGET  := review_intake
SRC     := review_intake.c

CFLAGS  := -g -O0 -fno-stack-protector -no-pie -fno-pie \
           -Wno-deprecated-declarations
LDFLAGS := -no-pie -z execstack

.PHONY: all clean checksec run aslr-off help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo
	@echo "Built '$(TARGET)' (64-bit). Security properties:"
	@$(MAKE) --no-print-directory checksec

# Show the relevant binary protections using tools everyone has.
checksec: $(TARGET)
	@echo "---------------------------------------------"
	@printf "  Arch        : "; file $(TARGET) | grep -o '64-bit' | head -1
	@printf "  NX (stack)  : "; \
	  if readelf -W -l $(TARGET) | grep -q 'GNU_STACK.*RWE'; then \
	    echo "DISABLED (stack executable)  <-- enables Stage 1 shellcode injection"; \
	  else echo "ENABLED  (stack non-executable)"; fi
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

# Launch a shell with ASLR disabled for this process tree (useful for manual
# gdb exploration). The exploit script does not need this -- it disables ASLR
# per-process itself via pwntools (aslr=False).
aslr-off:
	@echo "Starting a shell with ASLR disabled (type 'exit' to leave)..."
	setarch $$(uname -m) -R bash

clean:
	rm -f $(TARGET)

help:
	@echo "make            build the vulnerable binary (NX OFF, no canary, no PIE)"
	@echo "make checksec   show the binary's security properties"
	@echo "make run        run the app normally"
	@echo "make aslr-off   open a sub-shell with ASLR disabled (manual gdb)"
	@echo "make clean      remove the binary"
