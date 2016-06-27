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

#include <nmealib/validate.h>

#include <nmealib/context.h>
#include <ctype.h>

/** Invalid NMEA character: non-ASCII */
static const NmeaInvalidCharacter invalidNonAsciiCharsName = {
    .character = '*', //
    .description = "non-ASCII character" //
    };

/** Invalid NMEA characters */
static const NmeaInvalidCharacter invalidCharacters[] = {
    {
        .character = '$', //
        .description = "sentence delimiter" //
    },
    {
        .character = '*', //
        .description = "checksum field delimiter" //
    },
    {
        .character = '!', //
        .description = "exclamation mark" //
    },
    {
        .character = '\\', //
        .description = "backslash" //
    },
    {
        .character = '^', //
        .description = "power" //
    },
    {
        .character = '~', //
        .description = "tilde" //
    },
    {
        .character = '\0', //
        .description = NULL //
    }//
};

const NmeaInvalidCharacter * nmeaValidateIsInvalidCharacter(const char * c) {
  size_t i = 0;
  char ch;

  if (!c) {
    return NULL;
  }

  ch = *c;

  if ((ch < 32) || (ch > 126)) {
    return &invalidNonAsciiCharsName;
  }

  while (invalidCharacters[i].description) {
    if (ch == invalidCharacters[i].character) {
      return &invalidCharacters[i];
    }

    i++;
  }

  return NULL;
}

const NmeaInvalidCharacter * nmeaValidateSentenceHasInvalidCharacters(const char * s, const size_t sz) {
  size_t i = 0;

  if (!s || !sz) {
    return NULL;
  }

  for (i = 0; i < sz; i++) {
    const NmeaInvalidCharacter * invalid = nmeaValidateIsInvalidCharacter(&s[i]);
    if (invalid) {
      return invalid;
    }
  }

  return NULL;
}

bool nmeaValidateTime(const nmeaTIME * t, const char * prefix, const char * s) {
  if (!t) {
    return false;
  }

  if (!( //
  (t->hour >= 0) && (t->hour <= 23) //
      && (t->min >= 0) && (t->min <= 59) //
      && (t->sec >= 0) && (t->sec <= 60) //
      && (t->hsec >= 0) && (t->hsec <= 99))) {
    nmeaError("%s parse error: invalid time '%02d:%02d:%02d.%03d' (hh:mm:ss.mmm) in '%s'", prefix, t->hour, t->min,
        t->sec, t->hsec * 10, s);
    return false;
  }

  return true;
}

bool nmeaValidateDate(const nmeaTIME * t, const char * prefix, const char * s) {
  if (!t) {
    return false;
  }

  if (!( //
  (t->year >= 90) && (t->year <= 189) //
      && (t->mon >= 0) && (t->mon <= 11) //
      && (t->day >= 1) && (t->day <= 31))) {
    nmeaError("%s parse error: invalid date '%02d-%02d-%04d' (dd-mm-yyyy) in '%s'", prefix, t->day, t->mon,
        t->year + 1900, s);
    return false;
  }

  return true;
}

bool nmeaValidateNSEW(char * c, const bool ns, const char * prefix, const char * s) {
  char cu[3];

  if (!c) {
    return false;
  }

  cu[1] = '\0';
  cu[2] = '\0';
  if (*c) {
    *c = toupper(*c);
    cu[0] = *c;
  } else {
    cu[0] = '\\';
    cu[1] = '0';
  }

  if (ns) {
    if (!((*c == 'N') || (*c == 'S'))) {
      nmeaError("%s parse error: invalid North/South '%s' in '%s'", prefix, cu, s);
      return false;
    }
  } else {
    if (!((*c == 'E') || (*c == 'W'))) {
      nmeaError("%s parse error: invalid East/West '%s' in '%s'", prefix, cu, s);
      return false;
    }
  }

  return true;
}

bool nmeaValidateFix(int * fix, const char * prefix, const char * s) {
  if ((*fix < NMEA_FIX_FIRST) || (*fix > NMEA_FIX_LAST)) {
    nmeaError("%s parse error: invalid fix %d, expected [%d, %d] in '%s'", prefix, *fix, NMEA_FIX_FIRST, NMEA_FIX_LAST,
        s);
    return false;
  }

  return true;
}

bool nmeaValidateSignal(int * sig, const char * prefix, const char * s) {
  if ((*sig < NMEA_SIG_FIRST) || (*sig > NMEA_SIG_LAST)) {
    nmeaError("%s parse error: invalid signal %d, expected [%d, %d] in '%s'", prefix, *sig, NMEA_SIG_FIRST,
    NMEA_SIG_LAST, s);
    return false;
  }

  return true;
}

bool nmeaValidateMode(char * c, const char * prefix, const char * s) {
  if (!c) {
    return false;
  }

  *c = toupper(*c);

  if (!( //
  (*c == 'N') //
  || (*c == 'A') //
      || (*c == 'D') //
      || (*c == 'P') //
      || (*c == 'R') //
      || (*c == 'F') //
      || (*c == 'E') //
      || (*c == 'M') //
      || (*c == 'S'))) {
    nmeaError("%s parse error: invalid mode '%c' in '%s'", prefix, *c, s);
    return false;
  }

  return true;
}
