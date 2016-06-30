#ifndef _NMEALIB_RANDOM_H
#define _NMEALIB_RANDOM_H

#include <nmealib/util.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NMEALIB_RANDOM_MAX LONG_MAX

static INLINE double nmea_random(const double min, const double max) {
  long value;
  int randomFile;
  double range = fabs(max - min);

#ifdef _WIN32
  value = random();
#else

  randomFile = open("/dev/urandom", O_RDONLY);
  if (randomFile == -1) {
    randomFile = open("/dev/random", O_RDONLY);
  }

  if ((randomFile == -1) || (read(randomFile, &value, sizeof(value)) != sizeof(value))) {
    value = random();
  }

  if (randomFile != -1) {
    close(randomFile);
  }
#endif /* _WIN32 */

  return min + ((fabs((double) value) * range) / (double) NMEALIB_RANDOM_MAX);
}

static INLINE void nmea_init_random(void) {
  srandom((unsigned int) time(NULL));
}

#endif /* _NMEALIB_RANDOM_H */
