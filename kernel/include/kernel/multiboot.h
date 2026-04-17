#ifndef _KERNEL_MULTIBOOT_H
#define _KERNEL_MULTIBOOT_H

#include <stdint.h>

/* Bit 6 of flags: mmap_addr and mmap_length are valid */
#define MULTIBOOT_FLAG_MMAP (1 << 6)

/* Type 1 = usable RAM; all other types are reserved/off-limits */
#define MULTIBOOT_MEMORY_AVAILABLE 1

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;     /* KB of lower memory (below 1 MiB) */
    uint32_t mem_upper;     /* KB of upper memory (above 1 MiB) */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;   /* byte length of the mmap buffer */
    uint32_t mmap_addr;     /* physical address of first mmap entry */
} multiboot_info_t;

typedef struct multiboot_mmap_entry {
    uint32_t size;  /* size of this entry, NOT including this field (usually 20) */
    uint64_t addr;  /* base physical address of the region */
    uint64_t len;   /* length in bytes of the region */
    uint32_t type;  /* 1 = usable RAM */
} __attribute__((packed)) multiboot_mmap_entry_t;

#endif
