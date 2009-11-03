/* Verify memcpy operation.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <libitm.h>

/* Use a non-transactional destination so that we can get away with
   testing the results inside the transaction and not have to worry
   about a possible write-back cache.  */
#define MEMCPY	_ITM_memcpyRtWn

static unsigned char *buf1, *buf2;
static size_t bufsize, page_size;
static int fail;

static void
do_test (size_t align1, size_t align2, size_t len)
{
  size_t i, j;
  unsigned char *s1, *s2;
  unsigned char c1, c2;

  if (align1 + len >= bufsize)
    return;
  if (align2 + len >= bufsize)
    return;

  c1 = random () >> 8;
  c2 = random () >> 8;
  memset (buf1, c1, bufsize);
  memset (buf2, c2, bufsize);

  s1 = buf1 + align1;
  s2 = buf2 + align2;

  for (i = 0, j = 1; i < len; i++, j += 23)
    s1[i] = (j == c1 ? j + 1 : j);

  MEMCPY (s2, s1, len);

  if (memcmp (s1, s2, len) != 0)
    {
      printf ("Wrong result: dalign %zd salign %zd len %zd\n",
	      align2, align1, len);
      fail = 1;
      return;
    }

  for (i = (align2 > 64 ? align2 - 64 : 0); i < align2; ++i)
    if (buf2[i] != c2)
      {
	printf ("Garbage before: ofs %zd\n", i);
        fail = 1;
	break;
      }
  for (i = align2 + len, j = i+64 < bufsize ? i+64 : bufsize; i < j; ++i)
    if (buf2[i] != c2)
      {
	printf ("Garbage after: ofs %zd\n", i);
        fail = 1;
	break;
      }
}

int main()
{
  size_t i, j;

  page_size = getpagesize ();
  bufsize = 2 * page_size;

  buf1 = mmap (NULL, bufsize + 2*page_size, PROT_READ | PROT_WRITE,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf1 == MAP_FAILED)
    return 1;
  buf2 = mmap (NULL, bufsize + 2*page_size, PROT_READ | PROT_WRITE,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf2 == MAP_FAILED)
    return 1;

  if (mprotect (buf1, page_size, PROT_NONE))
    return 1;
  buf1 += page_size;
  if (mprotect (buf1 + bufsize, page_size, PROT_NONE))
    return 1;

  if (mprotect (buf2, page_size, PROT_NONE))
    return 1;
  buf2 += page_size;
  if (mprotect (buf2 + bufsize, page_size, PROT_NONE))
    return 1;

  _ITM_beginTransaction (pr_instrumentedCode | pr_hasNoAbort
			 | pr_hasNoIrrevocable);

  for (i = 0; i < 18; ++i)
    {
      size_t len = 1 << i;

      do_test (0, 0, len);
      do_test (i, 0, len);
      do_test (0, i, len);
      do_test (i, i, len);

      do_test (0, bufsize - len, len);
      do_test (bufsize - len, 0, len);
      do_test (i, bufsize - len, len);
      do_test (bufsize - len, i, len);
    }

  for (i = 0; i < 32; ++i)
    {
      do_test (i, 0, i);
      do_test (0, i, i);
      do_test (i, i, i);

      for (j = 0; j < 32; ++j)
	{
	  do_test (i, bufsize - i - j, i);
	  do_test (bufsize - i - j, i, i);
	}
    }

  for (i = 3; i < 32; ++i)
    {
      if ((i & (i - 1)) == 0)
	continue;
      do_test (0, 0, 16 * i);
      do_test (i, 0, 16 * i);
      do_test (0, i, 16 * i);
      do_test (i, i, 16 * i);
    }

  _ITM_commitTransaction ();

  return fail;
}
