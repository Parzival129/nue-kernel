#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/idt.h>
#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

static bool shift_pressed = false;

// IMPLEMENT PS/2 scancode to ASCII conversion table

// Scan Code Set 1: Typical US QWERTY Mapping
const char ascii_map[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

const char ascii_shift_map[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

// convert ps2 scancodes to decimal ascii using defined ascii maps

char ps2_to_ascii(uint8_t scancode)
{
    // Check for Break codes (Key Released)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {
            shift_pressed = false;
        }
        return 0; // Don't print anything on release
    }

    // Check for Make codes (Key Pressed)
    switch (scancode) {
    case 0x2A: // Left Shift
    case 0x36: // Right Shift
        shift_pressed = true;
        return 0;

    default:
        // Ensure scancode is within our table range
        if (scancode < sizeof(ascii_map)) {
            return shift_pressed ? ascii_shift_map[scancode] : ascii_map[scancode];
        }
        return 0;
    }
}

// Additional helper functionality (scrolling)

void scrollup(void)
{
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index - VGA_WIDTH] = terminal_buffer[index];
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

// Basic functions for terminal I/O

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

// moves the cusor to the given cursor location
// column, row
void update_cursor(size_t x, size_t y)
{
    uint16_t pos = y * VGA_WIDTH + x;
    // x86 assembly to perform change
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c)
{
    unsigned char uc = c;

    if (c == '\b') {
        // Backspace: move cursor back and erase
        if (terminal_column > 0) {
            terminal_column--;
        } else if (terminal_row > 0) {
            // Wrap to end of previous line
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
        }
        // Erase the character at the new cursor position
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    } else if (c == '\n') {
        // Newline: move to next line
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            scrollup();
            terminal_row = VGA_HEIGHT - 1;
        }
    } else if (c == '\t') {
        // Tab: advance to next multiple of 8
        terminal_column += 8 - (terminal_column % 8);
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                scrollup();
                terminal_row = VGA_HEIGHT - 1;
            }
        }
    } else {
        // Regular character: print and advance cursor
        terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                scrollup();
                terminal_row = VGA_HEIGHT - 1;
            }
        }
    }
}

void terminal_clear() {
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    update_cursor(0,0);
}

void terminal_write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
        update_cursor(terminal_column, terminal_row); // moves the cursor to the new location after placing character
    }
}

void terminal_writestring(const char* data)
{
    terminal_write(data, strlen(data));
}

size_t terminal_get_row(void)
{
    return terminal_row;
}

size_t terminal_get_column(void)
{
    return terminal_column;
}
