/*
 * This file is part of nmealib.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <nmealib/parse.h>

#include <nmealib/context.h>
#include <nmealib/tok.h>
#include <string.h>

bool nmeaTIMEparseTime(const char *time, nmeaTIME *t, const char *prefix, const char *s) {
  size_t sz;

  if (!time || !t) {
    return false;
  }

  sz = strlen(time);

  if (sz == 6) { // HHMMSS
    t->hsec = 0;
    return (3 == nmea_scanf(time, sz, "%2d%2d%2d", &t->hour, &t->min, &t->sec));
  }

  if (sz == 8) { // HHMMSS.t
    if (4 == nmea_scanf(time, sz, "%2d%2d%2d.%d", &t->hour, &t->min, &t->sec, &t->hsec)) {
      t->hsec *= 10;
      return true;
    }
    return false;
  }

  if (sz == 9) { // HHMMSS.hh
    return (4 == nmea_scanf(time, sz, "%2d%2d%2d.%d", &t->hour, &t->min, &t->sec, &t->hsec));
  }

  if (sz == 10) { // HHMMSS.mmm
    if ((4 == nmea_scanf(time, sz, "%2d%2d%2d.%d", &t->hour, &t->min, &t->sec, &t->hsec))) {
      t->hsec = (t->hsec + 5) / 10;
      return true;
    }
    return false;
  }

  nmea_error("%s%sparse error: invalid time format in '%s'", prefix ? prefix : "", prefix ? " " : "", s);

  return false;
}

bool nmeaTIMEparseDate(const int date, nmeaTIME *t, const char *prefix, const char *s) {
  if (!t) {
    return false;
  }

  if ((date < 0) || (date > 999999)) {
    nmea_error("%s%sparse error: invalid date '%d' in '%s'", prefix ? prefix : "", prefix ? " " : "", date, s);
    return false;
  }

  t->day = (date / 10000) % 100;
  t->mon = (date / 100) % 100;
  t->year = date % 100;

  t->mon--;

  if (t->year < 90) {
    t->year += 100;
  }

  return true;
}
