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

#include <nmealib/parser.h>

#include <nmealib/sentence.h>
#include <nmealib/tok.h>
#include <nmealib/validate.h>

#include <assert.h>
#include <ctype.h>
#include <string.h>

#define first_eol_char  ('\r')
#define second_eol_char ('\n')

static void reset_sentence_parser(nmeaPARSER * parser, sentence_parser_state new_state) {
  assert(parser);
  memset(&parser->sentence_parser, 0, sizeof(parser->sentence_parser));
  parser->buffer.buffer[0] = '\0';
  parser->buffer.length = 0;
  parser->sentence_parser.has_checksum = false;
  parser->sentence_parser.state = new_state;
}

static INLINE bool isHexChar(char c) {
  switch (tolower(c)) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      return true;

    default:
      break;
  }

  return false;
}

/**
 * Initialise the parser.
 * Allocates a buffer.
 *
 * @param parser a pointer to the parser
 * @return true (1) - success or false (0) - fail
 */
int nmea_parser_init(nmeaPARSER *parser) {
  assert(parser);
  reset_sentence_parser(parser, SKIP_UNTIL_START);
  return 1;
}

static bool nmea_parse_sentence_character(nmeaPARSER *parser, const char * c) {
  assert(parser);

  /* always reset when we encounter a start-of-sentence character */
  if (*c == '$') {
    reset_sentence_parser(parser, READ_SENTENCE);
    parser->buffer.buffer[parser->buffer.length++] = *c;
    return false;
  }

  /* just return when we haven't encountered a start-of-sentence character yet */
  if (parser->sentence_parser.state == SKIP_UNTIL_START) {
    return false;
  }

  /* this character belongs to the sentence */

  /* check whether the sentence still fits in the buffer */
  if (parser->buffer.length >= SENTENCE_SIZE) {
    reset_sentence_parser(parser, SKIP_UNTIL_START);
    return false;
  }

  parser->buffer.buffer[parser->buffer.length++] = *c;

  switch (parser->sentence_parser.state) {
    case READ_SENTENCE:
      if (*c == '*') {
        parser->sentence_parser.state = READ_CHECKSUM;
        parser->sentence_parser.sentence_checksum_chars_count = 0;
      } else if (*c == first_eol_char) {
        parser->sentence_parser.state = READ_EOL;
        parser->sentence_parser.sentence_eol_chars_count = 1;
      } else if (nmeaValidateIsInvalidCharacter(*c)) {
        reset_sentence_parser(parser, SKIP_UNTIL_START);
      } else {
        parser->sentence_parser.calculated_checksum ^= (int) *c;
      }
      break;

    case READ_CHECKSUM:
      if (!isHexChar(*c)) {
        reset_sentence_parser(parser, SKIP_UNTIL_START);
      } else {
        switch (parser->sentence_parser.sentence_checksum_chars_count) {
          case 0:
            parser->sentence_parser.sentence_checksum_chars[0] = *c;
            parser->sentence_parser.sentence_checksum_chars[1] = 0;
            parser->sentence_parser.sentence_checksum_chars_count = 1;
            break;

          case 1:
            parser->sentence_parser.sentence_checksum_chars[1] = *c;
            parser->sentence_parser.sentence_checksum_chars_count = 2;
            parser->sentence_parser.sentence_checksum = nmeaStringToInteger(parser->sentence_parser.sentence_checksum_chars, 2,
                16);
            parser->sentence_parser.has_checksum = true;
            parser->sentence_parser.state = READ_EOL;
            break;

          default:
            reset_sentence_parser(parser, SKIP_UNTIL_START);
            break;
        }
      }
      break;

    case READ_EOL:
      switch (parser->sentence_parser.sentence_eol_chars_count) {
        case 0:
          if (*c != first_eol_char) {
            reset_sentence_parser(parser, SKIP_UNTIL_START);
          } else {
            parser->sentence_parser.sentence_eol_chars_count = 1;
          }
          break;

        case 1:
          if (*c != second_eol_char) {
            reset_sentence_parser(parser, SKIP_UNTIL_START);
          } else {
            parser->sentence_parser.sentence_eol_chars_count = 2;

            /* strip off the end-of-line characters */
            parser->buffer.length -= parser->sentence_parser.sentence_eol_chars_count;
            parser->buffer.buffer[parser->buffer.length] = '\0';

            if (!parser->sentence_parser.has_checksum) {
              /* fake a checksum, needed for correct parsing of empty fields at the end of the sentence */
              parser->buffer.buffer[parser->buffer.length] = '*';
              parser->buffer.length++;
              parser->buffer.buffer[parser->buffer.length] = '\0';
            }

            parser->sentence_parser.state = SKIP_UNTIL_START;
            return (!parser->sentence_parser.sentence_checksum_chars_count
                || (parser->sentence_parser.sentence_checksum_chars_count
                    && (parser->sentence_parser.sentence_checksum == parser->sentence_parser.calculated_checksum)));
          }
          break;

        default:
          reset_sentence_parser(parser, SKIP_UNTIL_START);
          break;
      }
      break;

      /* can't occur, but keep compiler happy */
    case SKIP_UNTIL_START:
    default:
      break;

  }

  return false;
}

/**
 * Parse NMEA sentences from a (string) buffer and store the results in the nmeaINFO structure
 *
 * @param parser The parser
 * @param s The (string) buffer
 * @param len The length of the string in the buffer
 * @param info The nmeaINFO structure in which to store the information
 * @return The number of sentences that were parsed
 */
size_t nmea_parse(nmeaPARSER * parser, const char * s, size_t len, NmeaInfo * info) {
  size_t sentences_count = 0;
  size_t charIndex = 0;

  assert(parser);
  assert(s);
  assert(info);

  for (charIndex = 0; charIndex < len; charIndex++) {
    bool sentence_read_successfully = nmea_parse_sentence_character(parser, &s[charIndex]);
    if (sentence_read_successfully) {
      if (nmeaSentenceToInfo(parser->buffer.buffer, parser->buffer.length, info)) {
        sentences_count++;
      }
    }
  }

  return sentences_count;
}
