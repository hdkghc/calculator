/** @file src/syscalls.c
 *  @brief Newlib syscall stubs for bare-metal RT1011
 *  @author hdkghc
 *  @version 0.1
 *  Copyright (C) 2026 hdkghc (peitongxin@outlook.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

#undef errno
extern int errno;

/* ============================================================
 * System call stubs
 * ============================================================ */

int _write(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    return len;
}

int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    errno = ENOSYS;
    return -1;
}

int _close(int file) {
    (void)file;
    errno = ENOSYS;
    return -1;
}

int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    errno = ENOSYS;
    return -1;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

int _getentropy(void *buf, size_t len) {
    (void)buf;
    (void)len;
    errno = ENOSYS;
    return -1;
}

void _exit(int code) {
    (void)code;
    while (1) {
        __asm("bkpt #0");
    }
}

int __ssputws_r(void *reent, void *ptr, const wchar_t *str, size_t len) {
    (void)reent;
    (void)ptr;
    (void)str;
    (void)len;
    return (int)len;
}