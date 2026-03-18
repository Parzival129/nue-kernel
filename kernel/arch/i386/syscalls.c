// Implement POSIX-like stubs for the 3rd party C library to use

#include <stddef.h> // provides size_t and NULL
#include <stdint.h> // intptr_t and uint8_t
// #include <sys/stat.h> // gives structs
#include <kernel/tty.h> // get terminal_write
#include <string.h>

int errno; // will be intialized to 0 since its in BSS, set to associated error numbers when necessary
extern uint8_t _kernel_end[]; // for compiler to dereference where kernel ends
static uint8_t* heap_ptr = _kernel_end; // define where the heap begins

#define HEAP_MAX 0x100000 // 1 MiB
#define EBADF 9
#define S_IFCHR 0020000

struct stat {
    int st_mode;
};

void* sbrk(intptr_t increment) // move heap pointer
{
    uint8_t* prev = heap_ptr; // assign the current heap pointer to a temporary prev variable
    if (heap_ptr + increment > _kernel_end + HEAP_MAX) { // remember to start at _kernel_end memory address
        return (void*)-1; // out of heap space
    }
    heap_ptr += increment; // move the heap pointer
    return (void*)prev; // return the previous position as the start of the new block
}

int write(int fd, const char* buf, int len)
{ // write to the terminal
    if (fd == 1 || fd == 2) {
        terminal_write(buf, (size_t)len);
        return len;
    }
    errno = EBADF; // bad file descriptor error, "I don't know what file you're talking about"
    return -1; // error
}

int read(int fd, char* buf, int len)
{
    (void)fd; (void)buf; (void)len; // void unused args
    return -1; // not necessary at the moment since idt writes directly to the screen buffer, not file descriptor
}

// completely stops the kernel and halts the cpu indefinitely
__attribute__((noreturn)) void _exit(int status)
{ // signal that this function has no return val since it is the exit function
    (void)status;
    asm volatile ("cli; 1: hlt; jmp 1b");
    __builtin_unreachable();
}

int close(int fd)
{ // close a file descriptor
    (void)fd;
    return -1; // has nothing to close at the moment
}

int fstat(int fd, struct stat* st)
{
    (void)fd;
    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFCHR;
    return 0;
}

int isatty(int fd)
{ // checks if a file descriptor is tty
    if (fd == 0 || fd == 1 || fd == 2) {
        return 1;
    }
    return 0;
}

int lseek(int fd, int offset, int whence)
{ // can't seek on a character device like i386
    (void)fd; (void)offset; (void)whence;
    return -1;
}

int kill(int pid, int sig)
{ // no processes to signal in a single-task kernel
    (void)pid; (void)sig;
    return -1;
}

int getpid(void)
{ // always return 1, we're the only process
    return 1;
}
