
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

void printHexData(const char* descriptor, unsigned char *pcData, unsigned long iDataLen)
{
  unsigned int i;
  unsigned int iMaxLen;
  int iTail;

  unsigned char *pc;

  printf("\r\n%s - [%ld bytes]\r\n", descriptor, iDataLen);

  if (!iDataLen)
    return;

  iMaxLen = ((iDataLen - 1) / 16 + 1) * 16 + 1;

  pc = pcData;

  iTail = 0;
  for (i = 0; i < iMaxLen; i++)
  {
    /* Handling of content in ASCII display */
    if ((i & 0x0F) == 0 && i != 0)
    {
      /* Separator between hex and ASCII part of line */
      printf(" | ");

      while (pc < pcData)
      {
        /* Only printable chars to be displayed as ASCII char */
        if (*pc < 128 && *pc >= 32)
        {
          printf("%c", *pc++);
        }
        else
        {
          printf(".");
          pc++;
        }
      }

      /* Add blanks for each fillbyte */
      while (iTail > 0)
      {
        printf(" ");
        iTail--;
      }

      printf(" |\n");
    }

    /* Print out the content in hex display */
    if (i < iDataLen)
    {
      printf("%02x ", *pcData++);
    }
    else
    {
      /* Count number of Fillbytes */
      printf("   ");
      iTail++;
    } /* if ... else */
  } /* for */
} /* printHexData() */
