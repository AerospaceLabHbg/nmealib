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

/**
 * @file
 * NMEA Info
 *
 * The table below describes which fields are present in the sentences that are
 * supported by the library.
 * | field \ sentence       | GPGGA | GPGSA | GPGSV | GPRMC | GPVTG |
 * | :--------------------- | :---: | :---: | :---: | :---: | :---: |
 * | present:               | x     | x     | x     | x     | x     |
 * | smask:                 | x     | x     | x     | x     | x     |
 * | utc (date):            |       |       |       | x     |       |
 * | utc (time):            | x     |       |       | x     |       |
 * | sig:                   | x     |       |       | x (1) |       |
 * | fix:                   |       | x     |       | x (1) |       |
 * | PDOP:                  |       | x     |       |       |       |
 * | HDOP:                  | x     | x     |       |       |       |
 * | VDOP:                  |       | x     |       |       |       |
 * | lat:                   | x     |       |       | x     |       |
 * | lon:                   | x     |       |       | x     |       |
 * | elv:                   | x     |       |       |       |       |
 * | speed:                 |       |       |       | x     | x     |
 * | track:                 |       |       |       | x     | x     |
 * | mtrack:                |       |       |       |       | x     |
 * | magvar:                |       |       |       | x     |       |
 * | satinfo (inuse count): | x     | x (1) |       |       |       |
 * | satinfo (inuse):       |       | x     |       |       |       |
 * | satinfo (inview):      |       |       | x     |       |       |
 *
 * (1) Not present in the sentence but the library sets it up.
 */

#ifndef __NMEALIB_SENTENCE_H__
#define __NMEALIB_SENTENCE_H__

#include <stddef.h>

#include <nmealib/gpgga.h>
#include <nmealib/gpgsa.h>
#include <nmealib/gpgsv.h>
#include <nmealib/gprmc.h>
#include <nmealib/gpvtg.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Supported NMEA sentences
 */
enum NmeaSentence {
  GPNON = 0,
  GPGGA = (1u << 0),
  GPGSA = (1u << 1),
  GPGSV = (1u << 2),
  GPRMC = (1u << 3),
  GPVTG = (1u << 4),
  _NmeaSentenceLast = GPVTG
};

/** The fixed length of a NMEA prefix */
#define NMEA_PREFIX_LENGTH 5

/**
 * Determine the NMEA prefix from the sentence type.
 *
 * @param sentence The sentence type
 * @return The NMEA prefix, or NULL when the sentence type is unknown
 */
const char * nmeaSentenceToPrefix(enum NmeaSentence sentence);

/**
 * Determine the sentence type from the start of the specified string
 * (an NMEA sentence). The '$' character with which an NMEA sentence
 * starts must NOT be at the start of the specified string.
 *
 * @param s The string. Must be the NMEA string right after the initial '$'
 * character
 * @param sz The length of the string
 * @return The packet type, or GPNON when it could not be determined
 */
enum NmeaSentence nmeaPrefixToSentence(const char *s, const size_t sz);

/**
 * Parse a NMEA sentence into a nmeaINFO structure
 *
 * @param s The NMEA sentence
 * @param sz The length of the NMEA sentence
 * @param info The nmeaINFO structure in which to stored the information
 * @return True when successful
 */
bool nmeaSentenceToInfo(const char *s, const size_t sz, nmeaINFO * info);

/**
 * Generate a number of sentences from an nmeaINFO structure.
 *
 * @param s a pointer to the buffer in which to generate the sentences
 * @param len the size of the buffer
 * @param info the structure
 * @param generate_mask the mask of which sentences to generate
 * @return the total length of the generated sentences
 */
int nmea_generate(char *s, const int len, const nmeaINFO *info, const int generate_mask);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NMEALIB_SENTENCE_H__ */
