void swapped_intcpy( char* des, char* src, int size )
{
#if defined(__i386__)
  register int* d = (int*)des;
  register int* s = (int*)src;
  register int i = (size>>2)-1;
  if( i<0 ) return;
  do {
    asm ("movl %0,%%eax" :: "g" (*s) : "eax" );
    asm ("bswap %%eax" ::: "eax");
    asm ("movl %%eax,%0" : "=g" (*d) :: "eax" );
    s++; d++; i--;
  } while( i>=0 );
#else
  register int i = ((size>>2)<<2)-1;
  register int j;
  if( i<0 ) return;
  do {
    j=i-3;
    des[i--] = src[j++];
    des[i--] = src[j++];
    des[i--] = src[j++];
    des[i--] = src[j];
  } while( i>=0 );
#endif
}
