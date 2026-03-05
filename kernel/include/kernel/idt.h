#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#define NO_IDT_ENTRIES 256

#include <stdint.h>

// Creates a IDT entry, 8 bytes long

struct idt_entry {
   uint16_t base_low;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  zero;            // unused, set to 0
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t base_high;        // offset bits 16..31
} __attribute__((packed));

// struct pointer to IDT

struct idt_ptr {
    uint16_t limit; // limit size of the idt table (1 less than idt size in bytes)
    uint32_t base; // linear address of IDT (consider paging)
} __attribute__((packed)); // Tells the compiler to 'squish' these attributes together in memory with no 'padding' between them
// Avoids problems with CPU confusion when reading idt info

// Interrupt frame structure - represents the stack state when an interrupt occurs
struct interrupt_frame {
    uint32_t ds;          // Data segment selector
    uint32_t edi;         // General purpose registers
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;      // Interrupt number
    uint32_t err_code;    // Error code (pushed by CPU or dummy 0)
    uint32_t eip;         // Instruction pointer
    uint32_t cs;          // Code segment selector
    uint32_t eflags;      // EFLAGS register
} __attribute__((packed));

// Function to initialize idt from kernel_main
void idt_install(void);

// High-level interrupt handler
void isr_handler(struct interrupt_frame *frame);

void outb(uint16_t port, uint8_t value);

#endif