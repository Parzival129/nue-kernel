#include <stdio.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/multiboot.h>
#include <kernel/pmm.h>
#include <kernel/shell.h>
#include <kernel/tty.h>

void kernel_main(uint32_t multiboot_info_addr) // accepts multiboot info address from boot.S, use this information to create the memory bitmap (what is available and what isn't)
{

    terminal_initialize();
    setvbuf(stdout, NULL, _IONBF, 0); // disable stdout buffering so putchar writes immediately
    printf("[OK] terminal initialized\n");
    gdt_install();
    printf("[OK] gdt installed\n");
    idt_install();
    printf("[OK] idt installed\n");

    printf("                   __                    __\n");
    printf("  ___  __ _____   / /_____ _______  ___ / /\n");
    printf(" / _ \\/ // / -_) /  '_/ -_) __/ _ \\/ -_) / \n");
    printf("/_//_/\\_,_/\\__/ /_/\\_\\\\__/_/ /_//_/\\__/_/  \n");

    printf("booted\n\n");
    printf("READY\n");
    shell_init();

    // Tests the IDT exception handler with a division by 0 exception
    // __asm__ volatile ("movl $1, %%eax; xorl %%edx, %%edx; movl $0, %%ecx; divl %%ecx" ::: "eax", "ecx", "edx");

    for (;;) { // halts the program so it can observe IRQ interrupts for keyboard testing
        __asm__ volatile("hlt");
    }
}
