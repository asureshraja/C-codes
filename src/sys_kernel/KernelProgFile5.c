#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
int main ( )
{
 char foo[200], bar[200], baz[219];
 struct iovec iov[10];
 ssize_t nr;
 int fd, i;
 fd = open ("/home/suresh/Desktop/test.txt", O_RDONLY);
 if (fd == -1) {
 perror ("open");
 return 1;
 }
 /* set up our iovec structures */
 iov[0].iov_base = foo;
 iov[0].iov_len = sizeof (foo);
 iov[1].iov_base = bar;
 iov[1].iov_len = sizeof (bar);
 iov[2].iov_base = baz;
 iov[2].iov_len = sizeof (baz);
 /* read into the structures with a single call */
 nr = readv (fd, iov, 3);
 if (nr == -1) {
 perror ("readv");
 return 1;
 }
 for (i = 0; i < 3; i++)
 printf ("%d: %s", i, (char *) iov[i].iov_base);
 if (close (fd)) {
 perror ("close");
 return 1;
 }
 return 0;
}
