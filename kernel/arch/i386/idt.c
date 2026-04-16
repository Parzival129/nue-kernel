#include <kernel/idt.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h> // test comment to test computer push

struct idt_entry idt[NO_IDT_ENTRIES]; // 256 descriptors to fulfill i386 arch.
struct idt_ptr ip; // pointer to idt

extern void idt_init(uint32_t);

// Exception and interrupt handler stubs, based off i386 standards
// Implemented in isr.s

extern void isr0(void); // Division by Zero
extern void isr1(void); // Debug
extern void isr2(void); // Non-Maskable Interrupt
extern void isr3(void); // Breakpoint
extern void isr4(void); // Into Detected Overflow
extern void isr5(void); // Bound Range Exceeded
extern void isr6(void); // Invalid Opcode
extern void isr7(void); // Coprocessor not available
extern void isr8(void); // Double Fault
extern void isr9(void); // Coprocessor Segment Overrun
extern void isr10(void); // Invalid TSS
extern void isr11(void); // Segment Not Present
extern void isr12(void); // Stack-Segment Fault
extern void isr13(void); // General Protection Fault
extern void isr14(void); // Page Fault
extern void isr15(void); // Reserved
extern void isr16(void); // Floating Point Exception
extern void isr17(void); // Alignment Check
extern void isr18(void); // Machine Check
extern void isr19(void); // SIMD Floating-Point Exception
extern void isr20(void); // Virtualization Exception
extern void isr21(void); // Control Protection Exception
extern void isr22(void); // Reserved by CPU
extern void isr23(void); // Reserved
extern void isr24(void); // Reserved
extern void isr25(void); // Reserved
extern void isr26(void); // Reserved
extern void isr27(void); // Reserved
extern void isr28(void); // Reserved
extern void isr29(void); // Reserved
extern void isr30(void); // Security Exception
extern void isr31(void); // Reserved

// IRQ handlers for hardware interrupts: signal sent by hardware to cpu "I have data for you"
extern void irq0(void); // IRQ0 - Timer
extern void irq1(void); // IRQ1 - Keyboard
extern void irq2(void); // IRQ2 - Cascade
extern void irq3(void); // IRQ3 - COM2
extern void irq4(void); // IRQ4 - COM1
extern void irq5(void); // IRQ5 - LPT2
extern void irq6(void); // IRQ6 - Floppy
extern void irq7(void); // IRQ7 - LPT1
extern void irq8(void); // IRQ8 - Real Time Clock
extern void irq9(void); // IRQ9 - Peripheral
extern void irq10(void); // IRQ10 - Peripheral
extern void irq11(void); // IRQ11 - Peripheral
extern void irq12(void); // IRQ12 - Mouse
extern void irq13(void); // IRQ13 - Coprocessor
extern void irq14(void); // IRQ14 - ATA Primary
extern void irq15(void); // IRQ15 - ATA Secondary

// Functions for direct hardware communication:

// writes one byte of data to a specific hardware I/O port
void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// read one byte of data from a specific hardware I/O port
uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Creates a tiny delay to allow hardware to react to a previous I/O command
static inline void io_wait(void)
{
    outb(0x80, 0); // fake write that takes about 1 microsecond
}

// Remaps the PIC to move hardware interrupts from their conflicting default range (0x08–0x0F) to a safe range (usually 0x20+)
// prevents hardware IRQs from being misinterpreted by the CPU as internal exceptions or faults, normally not a problem in real mode but will interfere in protected mode (how the kernel runs)

// this function and its helpers were implemented with the help of copilot to get user input up and running
static void pic_remap(uint8_t master_offset, uint8_t slave_offset)
{
    uint8_t master_mask = inb(0x21);
    uint8_t slave_mask = inb(0xA1);

    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    outb(0x21, master_offset);
    io_wait();
    outb(0xA1, slave_offset);
    io_wait();

    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, master_mask);
    outb(0xA1, slave_mask);
}

// Tells the pic 'end of interrupt' and that it can accept more for the same hardware
static void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

void isr_handler(struct interrupt_frame* frame) // handles the interrupt service routines passed back from the stubs
// Uses two-stage assembly wrapping method (stubs defined in assembly file and handler function in C)
{
    if (frame->int_no < 32) { // the interrupt is for a execption
        // maps each isr to a exception message, matches idt defined below
        static const char* exception_messages[32] = {
            "Division By Zero",
            "Debug",
            "Non-Maskable Interrupt",
            "Breakpoint",
            "Overflow",
            "Bound Range Exceeded",
            "Invalid Opcode",
            "Device Not Available",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Invalid TSS",
            "Segment Not Present",
            "Stack-Segment Fault",
            "General Protection Fault",
            "Page Fault",
            "Reserved",
            "x87 Floating-Point Exception",
            "Alignment Check",
            "Machine Check",
            "SIMD Floating-Point Exception",
            "Virtualization Exception",
            "Control Protection Exception",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Security Exception",
            "Reserved"
        };

        printf("[EXCEPTION] #%lu: %s (err=0x%lx)\n",
            frame->int_no,
            exception_messages[frame->int_no],
            frame->err_code);

        for (;;) {
            __asm__ volatile("cli; hlt"); // halts everything since exceptions are unrecoverable at this stage
        }
    }

    if (frame->int_no >= 32 && frame->int_no <= 47) { // the interrupt is from hardware (interrupt request (IRQ))
        uint8_t irq = (uint8_t)(frame->int_no - 32);

        if (irq == 1) { // if it is keyboard input
            uint8_t scancode = inb(0x60);
            char c = ps2_to_ascii(scancode); // convert the ps2 scancode to ascii character
            if (c != 0) {
                shell_on_char(c);
            }
        }

        // if (irq == 0) { // Makes sure the timer (IRQ0) is running, also confirms EOI is being sent so ticks are registered continously
        //     printf("[IRQ] Timer tick\n");
        // }

        pic_send_eoi(irq); // tells the pic 'end of interrupt', that the interrupt has completed and that it can accept more interrupts from the same hardware
    }
}

void idt_set_entry(int index, uint32_t base, uint16_t seg_sel, uint8_t attributes)
{
    idt[index].base_low = (base & 0xFFFF);
    idt[index].base_high = (base >> 16) & 0xFFFF;
    idt[index].selector = seg_sel;
    idt[index].zero = 0;
    idt[index].type_attributes = attributes; // 0x8E interrupt gate, 0x8F trap gate, 0x85 task gate
}

void idt_install()
{
    ip.limit = (sizeof(struct idt_entry) * NO_IDT_ENTRIES) - 1;
    ip.base = (uint32_t)&idt;

    // Set up CPU exceptions (0-31)
    // Includes index of IDT entry, address of interrupt handler code, the segment selector, and attributes
    // Reference 0x08 kernel code segment that was defined in the GDT -> knows what code segment to run the handler in
    idt_set_entry(0, (uint32_t)isr0, 0x08, 0x8E); // Division by Zero Exception
    idt_set_entry(1, (uint32_t)isr1, 0x08, 0x8E); // Debug Exception
    idt_set_entry(2, (uint32_t)isr2, 0x08, 0x8E); // NMI Exception
    idt_set_entry(3, (uint32_t)isr3, 0x08, 0x8E); // Breakpoint Exception
    idt_set_entry(4, (uint32_t)isr4, 0x08, 0x8E); // Into Detected Overflow Exception
    idt_set_entry(5, (uint32_t)isr5, 0x08, 0x8E); // Bound Range Exceeded Exception
    idt_set_entry(6, (uint32_t)isr6, 0x08, 0x8E); // Invalid Opcode Exception
    idt_set_entry(7, (uint32_t)isr7, 0x08, 0x8E); // Coprocessor Not Available Exception
    idt_set_entry(8, (uint32_t)isr8, 0x08, 0x8E); // Double Fault Exception
    idt_set_entry(9, (uint32_t)isr9, 0x08, 0x8E); // Coprocessor Segment Overrun Exception
    idt_set_entry(10, (uint32_t)isr10, 0x08, 0x8E); // Invalid TSS Exception
    idt_set_entry(11, (uint32_t)isr11, 0x08, 0x8E); // Segment Not Present Exception
    idt_set_entry(12, (uint32_t)isr12, 0x08, 0x8E); // Stack-Segment Fault Exception
    idt_set_entry(13, (uint32_t)isr13, 0x08, 0x8E); // General Protection Fault Exception
    idt_set_entry(14, (uint32_t)isr14, 0x08, 0x8E); // Page Fault Exception
    idt_set_entry(15, (uint32_t)isr15, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(16, (uint32_t)isr16, 0x08, 0x8E); // Floating Point Exception
    idt_set_entry(17, (uint32_t)isr17, 0x08, 0x8E); // Alignment Check Exception
    idt_set_entry(18, (uint32_t)isr18, 0x08, 0x8E); // Machine Check Exception
    idt_set_entry(19, (uint32_t)isr19, 0x08, 0x8E); // SIMD Floating-Point Exception
    idt_set_entry(20, (uint32_t)isr20, 0x08, 0x8E); // Virtualization Exception
    idt_set_entry(21, (uint32_t)isr21, 0x08, 0x8E); // Control Protection Exception
    idt_set_entry(22, (uint32_t)isr22, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(23, (uint32_t)isr23, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(24, (uint32_t)isr24, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(25, (uint32_t)isr25, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(26, (uint32_t)isr26, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(27, (uint32_t)isr27, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(28, (uint32_t)isr28, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(29, (uint32_t)isr29, 0x08, 0x8E); // Reserved Exception
    idt_set_entry(30, (uint32_t)isr30, 0x08, 0x8E); // Security Exception
    idt_set_entry(31, (uint32_t)isr31, 0x08, 0x8E); // Reserved Exception

    // Set up IRQ (Interrupt request) handlers (for hardware) (mapped to IDT 32-47 after PIC remapping)
    idt_set_entry(32, (uint32_t)irq0, 0x08, 0x8E); // IRQ0 - Timer
    idt_set_entry(33, (uint32_t)irq1, 0x08, 0x8E); // IRQ1 - Keyboard
    idt_set_entry(34, (uint32_t)irq2, 0x08, 0x8E); // IRQ2 - Cascade
    idt_set_entry(35, (uint32_t)irq3, 0x08, 0x8E); // IRQ3 - COM2
    idt_set_entry(36, (uint32_t)irq4, 0x08, 0x8E); // IRQ4 - COM1
    idt_set_entry(37, (uint32_t)irq5, 0x08, 0x8E); // IRQ5 - LPT2
    idt_set_entry(38, (uint32_t)irq6, 0x08, 0x8E); // IRQ6 - Floppy
    idt_set_entry(39, (uint32_t)irq7, 0x08, 0x8E); // IRQ7 - LPT1
    idt_set_entry(40, (uint32_t)irq8, 0x08, 0x8E); // IRQ8 - Real Time Clock
    idt_set_entry(41, (uint32_t)irq9, 0x08, 0x8E); // IRQ9 - Peripheral
    idt_set_entry(42, (uint32_t)irq10, 0x08, 0x8E); // IRQ10 - Peripheral
    idt_set_entry(43, (uint32_t)irq11, 0x08, 0x8E); // IRQ11 - Peripheral
    idt_set_entry(44, (uint32_t)irq12, 0x08, 0x8E); // IRQ12 - Mouse
    idt_set_entry(45, (uint32_t)irq13, 0x08, 0x8E); // IRQ13 - Coprocessor
    idt_set_entry(46, (uint32_t)irq14, 0x08, 0x8E); // IRQ14 - ATA Primary
    idt_set_entry(47, (uint32_t)irq15, 0x08, 0x8E); // IRQ15 - ATA Secondary

    // 8259 PIC is the chip between hardware and the CPU, (signal from hardare -> pic -> interrupt index -> cpu)
    pic_remap(0x20, 0x28); // remaps IRQ to correct IRQ handler within the idt
    outb(0x21, 0xFC); // masks everything on master except the timer and the keyboard
    outb(0xA1, 0xFF); // masks everything on slave

    idt_init((uint32_t)&ip);

    // Enable hardware interrupts after IDT + PIC are initialized.
    __asm__ volatile("sti"); // CPU will now respond to interrupts, previously they were ignored before setup was complete
}
