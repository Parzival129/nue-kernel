# nue-kernel

`nue-kernel` is a hobby i386 (32-bit x86) kernel built for learning low-level systems programming in C and assembly, following the OSDev-style freestanding toolchain approach.

The project currently boots with GRUB (Multiboot), initializes protected-mode descriptor tables, handles interrupts, and provides a VGA text terminal with PS/2 keyboard input and a tiny built-in command prompt.

## Current State (April 2026)

Implemented and working:

- Multiboot-compliant kernel image and GRUB boot flow
- Early boot stack setup and C runtime init handoff (`_start -> _init -> kernel_main`)
- VGA text terminal (80x25) with cursor updates, scrolling, tab, and backspace handling
- GDT setup with kernel and user segments (flat 4 GiB model)
- IDT setup for CPU exceptions (0-31) and remapped PIC IRQs (32-47)
- Keyboard IRQ handling (PS/2 set 1 scancode -> ASCII translation)
- Minimal interactive prompt (`>`), with simple `info` and `help` commands
- Freestanding syscall stubs (`write`, `sbrk`, `fstat`, etc.) to support C library/runtime needs

In progress / planned:

- Paging and virtual memory
- Physical memory manager
- More robust heap allocator
- Timer/RTC-driven timekeeping and scheduling foundations
- Filesystem support
- Ring 3 userspace and syscall boundary hardening

## Architecture Overview

### 1) Boot and Entry

- `kernel/arch/i386/boot.S`
  - Declares Multiboot header (`MAGIC`, `FLAGS`, `CHECKSUM`)
  - Allocates a 16 KiB bootstrap stack in `.bss`
  - Sets `esp`, pushes Multiboot info pointer (`ebx`), calls `_init`, then `kernel_main`
  - Falls into a safe `cli; hlt` loop if `kernel_main` returns

- `kernel/arch/i386/linker.ld`
  - Loads kernel at `1 MiB`
  - Places `.multiboot` early in `.text`
  - Aligns sections to 4 KiB boundaries
  - Exports `_kernel_end` used by early heap growth (`sbrk`)

### 2) Kernel Initialization

- `kernel/kernel/kernel.c`
  - Initializes terminal output
  - Disables `stdout` buffering for immediate console prints
  - Installs GDT and IDT
  - Prints boot banner and enters halt loop while interrupts drive input

Initialization order today:

1. `terminal_initialize()`
2. `gdt_install()`
3. `idt_install()`

### 3) Console / TTY Subsystem

- `kernel/arch/i386/tty.c`
  - VGA text mode output at `0xB8000`
  - Character rendering + cursor port I/O (`0x3D4`, `0x3D5`)
  - Handles control characters (`\n`, `\b`, `\t`) and scroll-up behavior
  - Includes PS/2 keyboard scancode translation tables (`ascii_map`, `ascii_shift_map`)
  - Maintains shift key state for uppercase/symbol conversion

### 4) Segmentation (GDT)

- `kernel/arch/i386/gdt.c` + `kernel/arch/i386/gdt_init.s`
  - Builds 5-entry GDT:
    - Null
    - Kernel code (`0x08`)
    - Kernel data (`0x10`)
    - User code (`0x18`)
    - User data (`0x20`)
  - Loads GDTR with `lgdt`
  - Reloads segment registers
  - Performs far jump to flush CS cache

### 5) Interrupts and IRQs (IDT + PIC + ISR)

- `kernel/arch/i386/idt.c`
  - Creates all 256 IDT entries, filling exception and IRQ vectors
  - Remaps 8259 PIC from legacy overlap range to `0x20-0x2F`
  - Masks PIC to keep active focus on timer + keyboard lines
  - Enables interrupts via `sti`

- `kernel/arch/i386/isr.s`
  - Macro-generated ISR/IRQ stubs
  - Shared `isr_common_handler` saves register state and calls C dispatcher
  - Returns with `iret`

Runtime behavior:

- Exceptions (`int_no < 32`) print a readable exception message and halt
- IRQ1 (keyboard) reads scancode from port `0x60`, translates to ASCII, updates CLI buffer, and dispatches tiny built-in commands
- End-of-interrupt signaling is sent back to PIC after IRQ handling

### 6) Runtime / Syscall Compatibility Layer

- `kernel/arch/i386/syscalls.c`
  - Provides minimal low-level stubs often expected by freestanding/newlib-style runtimes:
    - `write` routes fd `1/2` to terminal
    - `sbrk` grows from `_kernel_end` within a fixed early limit
    - `fstat`, `isatty`, `_exit`, `getpid`, etc.
  - This is an early compatibility layer, not a full POSIX syscall implementation

## Repository Layout

- `kernel/` - kernel sources, architecture code, linker script, headers
- `libc/` - freestanding library build pieces (`libk.a`)
- `sysroot/` - generated sysroot populated during build/install steps
- `isodir/` - GRUB ISO staging directory (generated)
- Top-level scripts automate clean/build/ISO/QEMU workflows

## Build and Run

Default target toolchain host is `i686-elf`.

Typical run path:

```sh
./run.sh
```

Which performs:

1. `./clean.sh`
2. `./build.sh`
3. `./iso.sh`
4. `./qemu.sh`

Manual key scripts:

- `headers.sh` - installs kernel headers into sysroot and installs newlib target artifacts if present
- `build.sh` - builds configured projects into sysroot
- `iso.sh` - creates GRUB bootable `nue_kernel.iso`
- `qemu.sh` - boots ISO with `qemu-system-i386` (resolved from host triplet)

## Notes on Scope and Maturity

This is still an early-stage kernel focused on foundational bring-up and learning. Core boot/interrupt/terminal paths are in place, but memory management, isolation, scheduling, and userspace execution are not complete yet.

The current design intentionally favors clarity and incremental progress over abstraction-heavy structure, so subsystems are straightforward to inspect and evolve.
