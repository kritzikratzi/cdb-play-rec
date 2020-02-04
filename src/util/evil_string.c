#include "evil_string.h"
#ifdef _MSC_VER
#include "stdint.h"

/*
 * bit related functions
 *
 */

int ffs(int i)
{
   int size;
   int x;

   if (!i) return 1;

   /* remove the sign bit */
   x = i & -i;
   size = sizeof(int) << 3;
   for (i = size; i > 0; --i, x <<= 1)
     if (x & (1 << (size - 1))) return i;

   return x;
}


char *
strrstr (const char *str, const char *substr)
{
  char *it;
  char *ret = 0;

  while ((it = strstr(str, substr)))
    ret = it;

  return ret;
}

int strcasecmp(const char *s1, const char *s2)
{
   return _stricmp(s1, s2);
}


char *strcasestr(const char *haystack, const char *needle)
{
   size_t length_needle;
   size_t length_haystack;
   size_t i;

   if (!haystack || !needle)
     return NULL;

   length_needle = strlen(needle);
   length_haystack = strlen(haystack) - length_needle + 1;

   for (i = 0; i < length_haystack; i++)
     {
        size_t j;

        for (j = 0; j < length_needle; j++)
          {
            unsigned char c1;
            unsigned char c2;

            c1 = haystack[i+j];
            c2 = needle[j];
            if (toupper(c1) != toupper(c2))
              goto next;
          }
        return (char *) haystack + i;
     next:
        ;
     }

   return NULL;
}

#endif