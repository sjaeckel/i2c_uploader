

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <linux/i2c-dev.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "printhex.h"
#include "rpiGpio.h"


typedef struct
{
  char Device[8];              //0x00 - 0x07
  char Version[8];             //0x08 - 0x0F
  uint8_t PageSize;               //0x10
  uint8_t unused;                 //0x11
  uint8_t FreePages;              //0x12
  uint8_t LastCommand;            //0x13
  uint8_t CommandCount;           //0x14
  uint8_t State;                  //0x15
  uint8_t Reserved[7];            //0x16 - 0x1C
  volatile uint8_t PageNo;        //0x1D
  volatile uint8_t CommandStart;  //0x1E
  volatile uint8_t Command;       //0x1F
  uint8_t Data[64];               //0x20 - 0x5F
} I2CRegister_t;

#define DBG_PRINTF(...)

/*==============================================================================
 _asc2hex()
 ==============================================================================*/
uint8_t
_asc2hex(uint8_t* pc_asc)
{
  uint8_t pc_hex = 0;
  int i;

  for (i = 0; i < 2; ++i)
    {
      uint8_t u8_char = pc_asc[i];
      pc_hex <<= 4;
      if ((u8_char >= '0') && (u8_char <= '9'))
        pc_hex |= u8_char - '0';
      else if ((u8_char >= 'a') && (u8_char <= 'f'))
        pc_hex |= u8_char - 'a' + 10;
      else if ((u8_char >= 'A') && (u8_char <= 'F'))
        pc_hex |= u8_char - 'A' + 10;
      else
        {
          printf("zomg\n");
          pc_hex = 0xff;
          break;
        } /* if ... else if ... else if ... else */
    } /* for */

  return pc_hex;
} /* _asc2hex() */
static uint8_t* line;
uint8_t* buf = NULL;

void
cleanUp(void)
{
  gpioI2cCleanup();
  gpioCleanup();
  if (buf)
    free(buf);
  if (line)
    free(line);
}

int
main(void)
{
  //while(readline(NULL));
  //return -1;
  line = NULL;
  buf = NULL;
  atexit(cleanUp);
  size_t buflen = 0;
  uint8_t type = 0;
  //uint8_t* line = ":100000003AC20000000000000000000000000000F4";
  while ((type == 0) && (line = (uint8_t*) readline(NULL )) != NULL )
    {
      unsigned int line_len = strlen((char*) line);
      DBG_PRINTF("strlen = %u\n", line_len);
      if (line_len == 0)
        continue;
      assert(line_len > 8);
      assert((char )line[0] == ':');
      DBG_PRINTF("header found\n");
      uint8_t len = _asc2hex(&line[1]);
      DBG_PRINTF("len = %d\n", len);
      uint16_t addr = _asc2hex(&line[3]) << 8 | _asc2hex(&line[5]);
      DBG_PRINTF("addr = 0x%04X\n", addr);
      type = _asc2hex(&line[7]);
      DBG_PRINTF("type = 0x%02X\n", type);
      /* string-length must be
       * (len(1) + addr(2) + type(1) + n(bytes[len]) + crc(1))*2 + ':' */
      assert(line_len == (5 + len) * 2 + 1);
      unsigned int checksum = 0;
      unsigned int n;
      for (n = 0; n < (line_len) / 2; ++n)
        {
          checksum += _asc2hex(&line[1 + (n * 2)]);
          DBG_PRINTF("0x%02X\n", _asc2hex(&line[1 + (n * 2)]));
        }
      DBG_PRINTF("checksum = %d, 0x%08X\n", checksum, checksum);
      assert((checksum & 0x0FF) == 0);
      buf = realloc(buf, buflen + len);
      DBG_PRINTF("realloc buffer len = %d\n", (uint32_t) (buflen + len));
        {
          uint8_t i;
          for (i = 0; i < len; ++i)
            {
              buf[buflen + i] = _asc2hex(&line[9 + (2 * i)]);
            }
        }
      buflen += len;

      free(line);
      line = NULL;
    } /* while */
  printHexData("buf", buf, buflen);
  fflush(stdout);

  assert(gpioSetup() == OK);
  assert(gpioI2cSetup(1) == OK);
  assert(gpioI2cSetClock(I2C_CLOCK_FREQ_MIN) == OK);
  assert(gpioI2cSet7BitSlave(0x0) == OK);
  uint8_t testbuf[10] = {};
  char* data;
  while ((data = readline(NULL)) == NULL)
    sleep(1);
  size_t datlen = strlen(data) > sizeof(testbuf) ? sizeof(testbuf) : strlen(data);
  memcpy(testbuf, data, datlen);
  testbuf[sizeof(testbuf)-1] = '\0';
  printf("send %d bytes, %s\n", datlen, testbuf);
  assert(gpioI2cWriteData(testbuf, datlen) == OK);


  free(buf);
  buf = NULL;
  return EXIT_SUCCESS;
}
