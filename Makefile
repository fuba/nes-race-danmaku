# EDGERACE - NES Racing Game Makefile
# Minimal build without external libraries

# Output
ROM = build/edgerace.nes

# Source files
C_SOURCE = src/main.c
ASM_SOURCE = src/crt0.s

# Graphics
CHR_ROM = build/tiles.chr

# Tools
CC = cc65
CA = ca65
LD = ld65

# Flags
CFLAGS = -Oi -t nes --add-source
AFLAGS = -t nes
LDFLAGS = -C src/nrom.cfg

# Default target
all: build_dir $(ROM)
	@echo "Copying to web/..."
	cp $(ROM) web/edgerace.nes
	@echo ""
	@echo "=== Build Complete ==="
	@ls -la $(ROM) web/edgerace.nes
	@echo ""
	@echo "Test with any NES emulator (FCEUX, Mesen, Nestopia, etc.)"

build_dir:
	mkdir -p build

# Generate CHR-ROM
$(CHR_ROM): tools/generate_chr.py
	python3 tools/generate_chr.py $@

# Compile main.c to assembly
build/main.s: src/main.c
	$(CC) $(CFLAGS) -o $@ $<

# Assemble main.s
build/main.o: build/main.s
	$(CA) $(AFLAGS) -o $@ $<

# Assemble crt0.s
build/crt0.o: src/crt0.s
	$(CA) $(AFLAGS) -o $@ $<

# Link and create ROM
$(ROM): build/crt0.o build/main.o $(CHR_ROM)
	@echo "Linking..."
	$(LD) $(LDFLAGS) -o build/prg.bin build/crt0.o build/main.o nes.lib
	@echo "Appending CHR-ROM..."
	cat build/prg.bin $(CHR_ROM) > $(ROM)

# Clean
clean:
	rm -rf build/

# Info
info: $(ROM)
	@echo "ROM: $(ROM)"
	@echo "Size: $$(stat -c%s $(ROM) 2>/dev/null || stat -f%z $(ROM)) bytes"
	@hexdump -C $(ROM) | head -2

.PHONY: all clean info build_dir
