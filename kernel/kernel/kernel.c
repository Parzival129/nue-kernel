#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

void kernel_main(void) {
	terminal_initialize(); printf("[OK] terminal initialized\n");
	gdt_install(); printf("[OK] gdt installed\n");
	idt_install(); printf("[OK] idt installed\n");

	printf("                   __                    __\n");
	printf("  ___  __ _____   / /_____ _______  ___ / /\n");
	printf(" / _ \\/ // / -_) /  '_/ -_) __/ _ \\/ -_) / \n");
	printf("/_//_/\\_,_/\\__/ /_/\\_\\\\__/_/ /_//_/\\__/_/  \n"); 
                                           
	printf("booted\n\n");
	printf("READY\n>");

	// Tests the IDT exception handler with a division by 0 exception
	// __asm__ volatile ("movl $1, %%eax; xorl %%edx, %%edx; movl $0, %%ecx; divl %%ecx" ::: "eax", "ecx", "edx");
	
	for (;;) { // halts the program so it can observe IRQ interrupts for keyboard testing
		__asm__ volatile ("hlt");
	}
}