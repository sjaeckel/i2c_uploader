

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

typedef struct
{
  char     Device[8];              //0x00 - 0x07
  char     Version[8];             //0x08 - 0x0F
  uint8_t  PageSize;               //0x10
  uint8_t  unused;                 //0x11
  uint8_t  FreePages;              //0x12
  uint8_t  LastCommand;            //0x13
  uint8_t  CommandCount;           //0x14
  uint8_t  State;                  //0x15
  uint8_t  Reserved[7];            //0x16 - 0x1C
  volatile uint8_t  PageNo;        //0x1D
  volatile uint8_t  CommandStart;  //0x1E
  volatile uint8_t  Command;       //0x1F
  uint8_t  Data[64];               //0x20 - 0x5F
} I2CRegister_t;

int8_t _readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data) {
    int8_t count = 0;
    int fd = open("/dev/i2c-0", O_RDWR);

    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return(-1);
    }
    if (ioctl(fd, I2C_SLAVE, devAddr) < 0) {
        fprintf(stderr, "Failed to select device: %s\n", strerror(errno));
        close(fd);
        return(-1);
    }
    if (write(fd, &regAddr, 1) != 1) {
        fprintf(stderr, "Failed to write reg: %s\n", strerror(errno));
        close(fd);
        return(-1);
    }
    count = read(fd, data, length);
    if (count < 0) {
        fprintf(stderr, "Failed to read device(%d): %s\n", count, strerror(errno));
        close(fd);
        return(-1);
    } else if (count != length) {
        fprintf(stderr, "Short read  from device, expected %d, got %d\n", length, count);
        close(fd);
        return(-1);
    }
    close(fd);

    return count;
}

int8_t _readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data) {
    return _readBytes(devAddr, regAddr, 1, data);
}

/** Write consecutive bytes of length(length) to 8-bit registers.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
int _writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data) {
    int8_t count = 0;
    uint8_t buf[128];
    int fd;

    if (length > 127) {
        fprintf(stderr, "Byte write count (%d) > 127\n", length);
        return(0);
    }

    fd = open("/dev/i2c-0", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return(0);
    }
    if (ioctl(fd, I2C_SLAVE, devAddr) < 0) {
        fprintf(stderr, "Failed to select device: %s\n", strerror(errno));
        close(fd);
        return(0);
    }
    buf[0] = regAddr;
    memcpy(buf+1,data,length);
    count = write(fd, buf, length+1);
    if (count < 0) {
        fprintf(stderr, "Failed to write device(%d): %s\n", count, strerror(errno));
        close(fd);
        return(0);
    } else if (count != length+1) {
        fprintf(stderr, "Short write to device, expected %d, got %d\n", length+1, count);
        close(fd);
        return(0);
    }
    close(fd);

    return 1;
}
int _writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t* data) {
    return _writeBytes(devAddr, regAddr, 1, data);
}


/*==============================================================================
  _asc2hex()
==============================================================================*/
uint8_t _asc2hex(uint8_t* pc_asc)
{
  uint8_t pc_hex = 0;
  int i;

  for (i = 0; i < 2; ++i) {
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
uint8_t* buf=NULL;

void cleanUp(void)
{
  if (line)
    free(line);
  if (buf)
    free(buf);
}

int main(void) {
  //while(readline(NULL));
  //return -1;
  line = NULL;
  buf = NULL;
  atexit (cleanUp);
  size_t buflen = 0;

  //uint8_t* line = ":100000003AC20000000000000000000000000000F4";
  while ((line = (uint8_t*)readline(NULL)) != NULL)
    {
      unsigned int line_len = strlen((char*)line);
      printf("strlen = %u\n", line_len);
      if (line_len == 0)
        continue;
      assert(line_len > 8);
      assert((char)line[0] == ':');
      printf("header found\n");
      uint8_t len = _asc2hex(&line[1]);
      printf("len = %d\n", len);
      uint16_t addr = _asc2hex(&line[3]) << 8 | _asc2hex(&line[5]);
      printf("addr = 0x%04X\n", addr);
      uint8_t type = _asc2hex(&line[7]);
      printf("type = 0x%02X\n", type);
      /* string-length must be
       * (len(1) + addr(2) + type(1) + n(bytes[len]) + crc(1))*2 + ':' */
      assert(line_len == (5 + len)*2+1);
      unsigned int checksum = 0;
      unsigned int n;
      for (n = 0; n < (line_len)/2; ++n) {
          checksum += _asc2hex(&line[1 + (n*2)]);
          printf("0x%02X\n", _asc2hex(&line[1 + (n*2)]));
      }
      printf("checksum = %d, 0x%08X\n", checksum, checksum);
      assert((checksum&0x0FF) == 0);
      buf=realloc(buf, buflen + len);
      printf("realloc buffer len = %d\n", (uint32_t)(buflen + len));
      {
        uint8_t i;
        for (i = 0; i < len; ++i) {
          buf[buflen + i] = _asc2hex(&line[9 + (2*i)]);
        }
      }
      buflen += len;

      free(line);
      line = NULL;
    } /* while */
  printHexData("buf", buf, buflen);
  free(buf);
  buf = NULL;
  return EXIT_SUCCESS;
}
