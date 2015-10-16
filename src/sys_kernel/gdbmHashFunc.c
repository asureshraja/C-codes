/*
int
_gdbm_hash (datum key)
{
 unsigned int value;	/* Used to compute the hash value.  */
 int   index;		/* Used to cycle through random values. */


 /* Set the initial value from key. */
 value = 0x238F13AF * key.dsize;
 for (index = 0; index < key.dsize; index++)
   value = (value + (key.dptr[index] << (index*5 % 24))) & 0x7FFFFFFF;

 value = (1103515243 * value + 12345) & 0x7FFFFFFF;

 /* Return the value. */
 return((int) value);
}
*/
