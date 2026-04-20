#include <kernel/shell.h>
#include <kernel/pmm.h>
#include <kernel/tty.h>
#include <stdio.h>
#include <string.h>

static char cli_buffer[256];
static unsigned char cli_buffer_index;

static void shell_print_prompt(void)
{
    printf(">");
}

static void shell_execute_command(void)
{
    cli_buffer[cli_buffer_index] = '\0';

    if (cli_buffer_index == 4 && memcmp(cli_buffer, "info", 4) == 0) {
        printf("Nue Kernel v0.1\n");

    } else if (cli_buffer_index == 5 && memcmp(cli_buffer, "clear", 5) == 0) {
        terminal_clear();

    } else if (cli_buffer_index == 4 && memcmp(cli_buffer, "help", 4) == 0) {
        printf("info - get kernel and version\n");
        printf("help - list commands\n");
        printf("clear - clear terminal\n");
        printf("mem - total and free memory\n");
        printf("reboot - reboot system\n");

    } else if (cli_buffer_index == 6 && memcmp(cli_buffer, "reboot", 6) == 0) {
        unsigned long long null_idtr = 0;
        __asm__ volatile("xor %%eax, %%eax; lidt %0; int3" : : "m"(null_idtr));

    // Memory test command 
    // } else if (cli_buffer_index == 3 && memcmp(cli_buffer, "pmm", 3) == 0) {
    //     printf("total frames:  %u\n", pmm_get_total_frame_count());
    //     printf("free frames:   %u\n", pmm_get_free_frame_count());
    //     uint32_t a = pmm_alloc_frame();
    //     uint32_t b = pmm_alloc_frame();
    //     uint32_t c = pmm_alloc_frame();
    //     printf("alloc 3 frames: 0x%x  0x%x  0x%x\n", a, b, c);
    //     pmm_free_frame(b);
    //     printf("freed middle frame (0x%x)\n", b);
    //     uint32_t d = pmm_alloc_frame();
    //     printf("next alloc: 0x%x (should match freed frame)\n", d);
    //     printf("free frames now: %u\n", pmm_get_free_frame_count());


    } else if (cli_buffer_index == 3 && memcmp(cli_buffer, "mem", 3) == 0) {
        printf("total frames:  %u\n", pmm_get_total_frame_count());
        printf("free frames:   %u\n", pmm_get_free_frame_count());
        printf("total memory:  %u MB\n", pmm_get_total_frame_count() / 256);
        printf("free memory:   %u MB\n", pmm_get_free_frame_count() / 256);
        printf("memory usage:  %u%%\n", ((pmm_get_total_frame_count() - pmm_get_free_frame_count()) * 100) / pmm_get_total_frame_count());

        
    } else if (cli_buffer[0] != '\0') {
        printf("command '%s' not recognized\n", cli_buffer);
    }

    cli_buffer_index = 0;
    cli_buffer[0] = '\0';
    shell_print_prompt();
}

void shell_init(void)
{
    cli_buffer_index = 0;
    cli_buffer[0] = '\0';
    shell_print_prompt();
}

void shell_on_char(char c)
{
    if (c == '\n') {
        putchar(c);
        shell_execute_command();
        return;
    }

    if (c == '\b') {
        if (cli_buffer_index > 0) {
            cli_buffer_index--;
            cli_buffer[cli_buffer_index] = '\0';
            putchar(c);
        }
        return;
    }

    if (cli_buffer_index < sizeof(cli_buffer) - 1) {
        cli_buffer[cli_buffer_index++] = c;
        cli_buffer[cli_buffer_index] = '\0';
        putchar(c);
    }
}
