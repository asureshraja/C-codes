/* For the size of the file. */
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void
check (int test, const char * message, ...)
{
    if (test) {
        va_list args;
        va_start (args, message);
        vfprintf (stderr, message, args);
        va_end (args);
        fprintf (stderr, "\n");
        exit (EXIT_FAILURE);
    }
}

int main ()
{
    /* The file descriptor. */

    /* Information about the file. */
    struct stat s;

    size_t size;
    /* The file name to open. */
    const char * file_name = "/home/suresh/Desktop/test.db";
    /* The memory-mapped thing itself. */
    int fd = open (file_name, O_RDONLY);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;
char *f;
    f = (char *) mmap (0, size, PROT_READ, MAP_SHARED, fd, 0);int i;
    for ( i = 0; i < size; i++) {
        char c;

        c = f[i];
        putchar(c);
    }

    return 0;
}
