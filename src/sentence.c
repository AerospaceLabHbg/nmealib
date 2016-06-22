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

#include <nmea/sentence.h>
#include <string.h>

/**
 * The type definition for an entry mapping a NMEA sentence prefix to a sentence type
 */
typedef struct {
  const char * prefix;
  const enum NmeaSentence sentenceType;
} SentencePrefixToType;

/**
 * The map from NMEA sentence prefix to sentence type
 */
static const SentencePrefixToType sentencePrefixToType[] = {
    {
        .prefix = "GPGGA",
        .sentenceType = GPGGA },
    {
        .prefix = "GPGSA",
        .sentenceType = GPGSA },
    {
        .prefix = "GPGSV",
        .sentenceType = GPGSV },
    {
        .prefix = "GPRMC",
        .sentenceType = GPRMC },
    {
        .prefix = "GPVTG",
        .sentenceType = GPVTG },
    {
        .prefix = NULL,
        .sentenceType = GPNON } };

const char * nmeaSentenceToPrefix(enum NmeaSentence sentence) {
  switch (sentence) {
    case GPGGA:
      return "GPGGA";

    case GPGSA:
      return "GPGSA";

    case GPGSV:
      return "GPGSV";

    case GPRMC:
      return "GPRMC";

    case GPVTG:
      return "GPVTG";

    case GPNON:
    default:
      return NULL;
  }
}

enum NmeaSentence nmeaPrefixToSentence(const char *s, const size_t sz) {
  size_t i = 0;
  const char * str = s;
  size_t size = sz;

  if (!s) {
    return GPNON;
  }

  if (*str == '$') {
    str++;
    size--;
  }

  if (sz < NMEA_PREFIX_LENGTH) {
    return GPNON;
  }

  while (sentencePrefixToType[i].prefix) {
    if (!strncmp(s, sentencePrefixToType[i].prefix, NMEA_PREFIX_LENGTH)) {
      return sentencePrefixToType[i].sentenceType;
    }

    i++;
  }

  return GPNON;
}
