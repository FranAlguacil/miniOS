# =============================================================================
# miniOS Makefile
#
# Required toolchain (install via WSL / MSYS2 / Linux):
#   nasm             -- assembler
#   i686-elf-gcc     -- bare-metal C compiler  (or gcc with -m32)
#   i686-elf-ld      -- bare-metal linker       (or ld with -m elf_i386)
#   i686-elf-objcopy -- ELF → flat binary       (or objcopy)
#   qemu-system-i386 -- emulator
#
# Ubuntu/Debian WSL setup:
#   sudo apt install nasm gcc-multilib binutils qemu-system-x86
#   (use CC=gcc CFLAGS_EXTRA="-m32" LD=ld LDFLAGS_EXTRA="-m elf_i386")
#
# Cross-compiler setup (recommended):
#   Build i686-elf toolchain from OSDev wiki or use a pre-built package.
# =============================================================================

# ── Toolchain ─────────────────────────────────────────────────────────────────
AS       := nasm
QEMU     := qemu-system-i386

# Auto-detect: prefer cross-compiler, fall back to host gcc -m32
ifneq ($(shell command -v i686-elf-gcc 2>/dev/null),)
    CC      := i686-elf-gcc
    LD      := i686-elf-ld
    OBJCOPY := i686-elf-objcopy
    CFLAGS  :=
    LDFLAGS :=
else
    CC      := gcc
    LD      := ld
    OBJCOPY := objcopy
    CFLAGS  := -m32
    LDFLAGS := -m elf_i386
endif

CFLAGS  += -ffreestanding -fno-stack-protector -fno-builtin \
           -nostdlib -nostdinc -Wall -Wextra -O2 \
           -Ikernel/include

LDFLAGS += -T linker.ld

ASFLAGS := -f elf32

# ── Targets ───────────────────────────────────────────────────────────────────
DISK_IMG := miniOS.img
DISK_SECTORS := 2880        # 1.44 MB

BUILD    := build

BOOT_SRC := boot/boot.asm
BOOT_BIN := $(BUILD)/boot.bin

KERNEL_ASM_SRCS := kernel/arch/x86/start.asm \
                   kernel/arch/x86/idt.asm
KERNEL_C_SRCS   := kernel/kernel.c \
                   kernel/drivers/vga.c \
                   kernel/drivers/keyboard.c \
                   kernel/arch/x86/idt.c \
                   kernel/arch/x86/pic.c

KERNEL_OBJS := \
    $(patsubst %.asm, $(BUILD)/%.o, $(KERNEL_ASM_SRCS)) \
    $(patsubst %.c,   $(BUILD)/%.o, $(KERNEL_C_SRCS))

KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin

# ── Rules ─────────────────────────────────────────────────────────────────────
.PHONY: all clean run run-debug

all: $(DISK_IMG)

# Boot sector (flat binary, must be exactly 512 bytes)
$(BOOT_BIN): $(BOOT_SRC)
	@mkdir -p $(dir $@)
	$(AS) -f bin $< -o $@
	@if [ $$(wc -c < $@) -ne 512 ]; then \
	    echo "ERROR: boot binary is not 512 bytes!"; exit 1; fi

# Assembly kernel objects
$(BUILD)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# C kernel objects
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel ELF, then strip to flat binary
$(KERNEL_ELF): $(KERNEL_OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# Disk image: 1.44 MB, boot at sector 0, kernel at sector 1
$(DISK_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	dd if=/dev/zero      of=$@ bs=512 count=$(DISK_SECTORS) 2>/dev/null
	dd if=$(BOOT_BIN)    of=$@ bs=512 count=1    conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN)  of=$@ bs=512 seek=1     conv=notrunc 2>/dev/null
	@echo ""
	@echo "  Disk image : $(DISK_IMG)"
	@echo "  Boot size  : $$(wc -c < $(BOOT_BIN)) bytes"
	@echo "  Kernel size: $$(wc -c < $(KERNEL_BIN)) bytes"
	@echo ""

# Run in QEMU
run: $(DISK_IMG)
	$(QEMU) \
	    -drive format=raw,file=$(DISK_IMG) \
	    -no-reboot -no-shutdown \
	    -serial stdio

# Run with GDB server for debugging
run-debug: $(DISK_IMG)
	$(QEMU) \
	    -drive format=raw,file=$(DISK_IMG) \
	    -no-reboot -no-shutdown \
	    -s -S

clean:
	rm -rf $(BUILD) $(DISK_IMG)
