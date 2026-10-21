#define _DEFAULT_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>

#include "randombytes.h"

#ifdef SYS_getrandom
static void randombytes_sys_getrandom(unsigned char *out, size_t outlen)
{
  ssize_t ret;
  while(outlen > 0)
  {
    ret = syscall(SYS_getrandom, out, outlen, 0);
    if(ret == -1 && errno == EINTR)
      continue;
    else if(ret <= 0)
      abort(); // Never continue key generation with unfilled randomness.

    out += ret;
    outlen -= (size_t)ret;
  }
}
#endif

void randombytes(unsigned char *out, size_t outlen)
{
#ifdef SYS_getrandom
  randombytes_sys_getrandom(out, outlen);
#else
  int fd = -1;
  ssize_t ret;
  while(fd == -1)
  {
    fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if(fd == -1 && errno == EINTR)
      continue;
    else if(fd == -1)
      abort();
  }

  while(outlen > 0)
  {
    ret = read(fd, out, outlen);
    if(ret == -1 && errno == EINTR)
      continue;
    else if(ret <= 0)
      abort();

    out += ret;
    outlen -= (size_t)ret;
  }
  close(fd);
#endif
}

uint32_t randombytes_uniform(uint32_t upper_bound)
{
  uint32_t r, min;

  if (upper_bound < 2) {
    return 0;
  }

  min = (uint32_t) -upper_bound % upper_bound;

  for (;;) {
    randombytes((unsigned char *)&r, sizeof(r));
    if (r >= min) {
      return r % upper_bound;
    }
  }
}
