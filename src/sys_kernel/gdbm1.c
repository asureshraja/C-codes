#include <stdio.h>
#include <gdbm.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int
main(void)
{
  GDBM_FILE dbf;
  datum key = { "testkey", 8 };     /* key, length */
  datum value = { "testvalue", 10 }; /* value, length */
  dbf = gdbm_open("/home/suresh/codes/Git/C-codes/src/sys_kernel/testdb", 0, GDBM_NEWDB,0666, 0);

  clock_t start, end;
  double cpu_time_used;



  start = clock();
  int i=0;
  while(i<1000000){
      gdbm_store (dbf, key, value, GDBM_INSERT);
      value = gdbm_fetch(dbf, key);
      i=i+1;
  }


  end = clock();

  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("%G\n",cpu_time_used );

  printf ("%s\n", value.dptr);

  free (value.dptr);
  gdbm_close (dbf);

   return 0;
}
