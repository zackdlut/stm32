/*
 * Minimal newlib system calls.
 *
 * newlib's printf eventually calls _write(fd, buf, len). On a PC the OS
 * implements it; here we forward every byte to __io_putchar(), which a lesson
 * or drivers/uart.c defines to push the byte out of a UART.
 */
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

__attribute__((weak)) int __io_putchar(int ch)
{
    return ch;
}

__attribute__((weak)) int __io_getchar(void)
{
    return -1;
}

int _write(int fd, const char *buf, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) {
        __io_putchar(buf[i]);
    }
    return len;
}

int _read(int fd, char *buf, int len)
{
    (void)fd;
    int n = 0;
    while (n < len) {
        int ch = __io_getchar();
        if (ch < 0) {
            break;
        }
        buf[n++] = (char)ch;
        if (ch == '\n') {
            break;
        }
    }
    return n;
}

int _close(int fd)
{
    (void)fd;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

int _lseek(int fd, int offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void)status;
    for (;;) {
    }
}

/* Heap for malloc: grows from `end` (linker script) toward the stack. */
void *_sbrk(int incr)
{
    extern uint8_t end;
    extern uint8_t _estack;
    extern uint32_t _min_stack_size;
    static uint8_t *heap_end;

    if (heap_end == 0) {
        heap_end = &end;
    }

    uint8_t *stack_limit = &_estack - (uint32_t)&_min_stack_size;
    if (heap_end + incr > stack_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }

    uint8_t *prev = heap_end;
    heap_end += incr;
    return prev;
}
