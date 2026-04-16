#include <kernel/shell.h>

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
        for (int i = 0; i < 80 * 25; i++) {
            printf(" ");
        }
    } else if (cli_buffer_index == 4 && memcmp(cli_buffer, "help", 4) == 0) {
        printf("There is no help here..\n");
    } else if (cli_buffer_index == 7 && memcmp(cli_buffer, "reboot?", 7) == 0) {
        unsigned long long null_idtr = 0;
        __asm__ volatile("xor %%eax, %%eax; lidt %0; int3" : : "m"(null_idtr));
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
