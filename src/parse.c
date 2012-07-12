/*
 * This file is part of nmealib.
 *
 * Copyright (c) 2008 Timur Sinitsyn
 * Copyright (c) 2011 Ferry Huberts
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

#include <nmea/parse.h>

#include <nmea/gmath.h>
#include <nmea/tok.h>
#include <nmea/context.h>

#include <string.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

/** the size of the buffer to put time string (that is to be parsed) into */
#define NMEA_TIMEPARSE_BUF  256

/**
 * Parse nmeaTIME (time only, no date) from a string.
 * The format that is used (hhmmss, hhmmss.s, hhmmss.ss or hhmmss.sss) is
 * determined by the length of the string.
 *
 * @param s the string
 * @param len the length of the string
 * @param t a pointer to the nmeaTIME structure in which to store the parsed time
 * @return true on success, false otherwise
 */
static bool _nmea_parse_time(const char *s, int len, nmeaTIME *t) {
	assert(s);
	assert(t);

	if (len == (sizeof("hhmmss") - 1)) {
		return (3 == nmea_scanf(s, len, "%2d%2d%2d", &t->hour, &t->min, &t->sec));
	}

	if (len == (sizeof("hhmmss.s") - 1)) {
		if (4 == nmea_scanf(s, len, "%2d%2d%2d.%d", &t->hour, &t->min, &t->sec, &t->hsec)) {
			t->hsec *= 10;
			return true;
		}
		return false;
	}

	if (len == (sizeof("hhmmss.ss") - 1)) {
		return (4 == nmea_scanf(s, len, "%2d%2d%2d.%d", &t->hour, &t->min, &t->sec, &t->hsec));
	}

	if (len == (sizeof("hhmmss.sss") - 1)) {
		if ((4 == nmea_scanf(s, len, "%2d%2d%2d.%d", &t->hour, &t->min, &t->sec, &t->hsec))) {
			t->hsec /= 10;
			return true;
		}
		return false;
	}

	nmea_error("Time parse error: invalid format in %s", s);
	return false;
}

/**
 * Validate the time fields in an nmeaTIME structure.
 * Expects:
 * <pre>
 *   0 <= hour <   24
 *   0 <= min  <   60
 *   0 <= sec  <=  60
 *   0 <= hsec <  100
 * </pre>
 *
 * @param t a pointer to the structure
 * @return true when valid, false otherwise
 */
static bool validateTime(nmeaTIME * t) {
	if (!t) {
		return false;
	}

	if (!((t->hour >= 0) && (t->hour < 24) && (t->min >= 0) && (t->min < 60) && (t->sec >= 0) && (t->sec <= 60)
			&& (t->hsec >= 0) && (t->hsec < 100))) {
		nmea_error("Parse error: invalid time (%d:%d:%d.%d)", t->hour, t->min, t->sec, t->hsec);
		return false;
	}

	return true;
}

/**
 * Validate the date fields in an nmeaTIME structure.
 * Expects:
 * <pre>
 *   year  [90, 189]
 *   month [ 0,  11]
 *   day   [ 1,  31]
 * </pre>
 *
 * @param t a pointer to the structure
 * @return true when valid, false otherwise
 */
static bool validateDate(nmeaTIME * t) {
	if (!t) {
		return false;
	}

	if (!((t->year >= 90) && (t->year <= 189) && (t->mon >= 0) && (t->mon <= 11) && (t->day >= 1) && (t->day <= 31))) {
		nmea_error("Parse error: invalid date (%d-%d-%d - D-M-Y)", t->day, t->mon, t->year);
		return false;
	}

	return true;
}

/**
 * Validate north/south or east/west and uppercase it.
 * Expects:
 * <pre>
 *   c in { n, N, s, S } (for north/south)
 *   c in { e, E, w, W } (for east/west)
 * </pre>
 *
 * @param c a pointer to the character. The character will be converted to uppercase.
 * @param ns true: evaluate north/south, false: evaluate east/west
 * @return true when valid, false otherwise
 */
static bool validateNSEW(char * c, bool ns) {
	if (!c) {
		return false;
	}

	*c = toupper(*c);

	if (ns) {
		if (!((*c == 'N') || (*c == 'S'))) {
			nmea_error("Parse error: invalid north/south (%c)", *c);
			return false;
		}
	} else {
		if (!((*c == 'E') || (*c == 'W'))) {
			nmea_error("Parse error: invalid east/west (%c)", *c);
			return false;
		}
	}

	return true;
}

/**
 * Validate mode and uppercase it.
 * Expects:
 * <pre>
 *   c in { a, A, d, D, e, E, n, N, s, S }
 * </pre>
 *
 * @param c a pointer to the character. The character will be converted to uppercase.
 * @return true when valid, false otherwise
 */
static bool validateMode(char * c) {
	if (!c) {
		return false;
	}

	*c = toupper(*c);

	if (!((*c == 'A') || (*c == 'D') || (*c == 'E') || (*c == 'N') || (*c == 'S'))) {
		nmea_error("Parse error: invalid mode (%c)", *c);
		return false;
	}

	return true;
}

/**
 * Determine whether the given string contains characters that are not allowed
 * for fields in an NMEA string.
 *
 * @param str
 * The string to check
 * @param strName
 * The name of the string to report when invalid characters are encountered
 * @param report
 * A pointer to a buffer in which to place the report string when an invalid
 * nmea character is detected
 * @param reportSize
 * The size of the report buffer
 *
 * @return
 * - true when the string has invalid characters
 * - false otherwise
 */
bool nmea_string_has_invalid_chars(const char * str, const char * strName, char * report, size_t reportSize) {
	static const char invalidChars[] = { '$', '*', ',', '!', '\\', '^', '~' };
	static const char * invalidCharsNames[] = { "sentence delimiter ($)", "checksum field delimiter (*)", "comma (,)",
			"exclamation mark (!)", "backslash (\\)", "power (^)", "tilde (~)" };

	size_t i;
	size_t j;
	size_t slen;

	if (!str) {
		return false;
	}

	slen = strlen(str);
	for (i = 0; i < slen; i++) {
		char c = str[i];

		if (!((c >= 32) && (c <= 126))) {
			if (report && reportSize) {
				snprintf(report, reportSize, "Configured %s (%s),"
						" character %lu, can not contain non-printable"
						" characters (codes outside the range [32, 126])", strName, str, (unsigned long) i + 1);
			}
			return true;
		}

		for (j = 0; j < sizeof(invalidChars); j++) {
			if (c == invalidChars[j]) {
				if (report && reportSize) {
					snprintf(report, reportSize, "Configured %s (%s),"
							" character %lu, can not contain %s characters", strName, str, (unsigned long) i + 1,
							invalidCharsNames[j]);
				}
				return true;
			}
		}
	}

	return false;
}

/**
 * Determine packet type (see nmeaPACKTYPE) by the header of a string.
 * The header is the start of an NMEA sentence, right after the $.
 *
 * @param s the string
 * @param len the length of the string
 * @return The packet type (or GPNON when it could not be determined)
 */
int nmea_pack_type(const char *s, int len) {
	static const char *pheads[] = { "GPGGA", "GPGSA", "GPGSV", "GPRMC", "GPVTG" };

	assert(s);

	if (len < 5)
		return GPNON;
	if (!memcmp(s, pheads[0], 5))
		return GPGGA;
	if (!memcmp(s, pheads[1], 5))
		return GPGSA;
	if (!memcmp(s, pheads[2], 5))
		return GPGSV;
	if (!memcmp(s, pheads[3], 5))
		return GPRMC;
	if (!memcmp(s, pheads[4], 5))
		return GPVTG;

	return GPNON;
}

/**
 * Find the tail ("\r\n") of a sentence in a string and check the checksum of the sentence.
 *
 * @param s the string
 * @param len the length of the string
 * @param checksum a pointer to the location where the checksum (as specified in the sentence) should be stored
 * (will be -1 if the checksum did not match the calculated checksum)
 * @return Number of bytes from the start of the string until the tail or 0 when the checksum did not match
 * the calculated checksum
 */
int nmea_find_tail(const char *s, int len, int *checksum) {
	static const int tail_sz = 1 + 2 + 2 /* *xx\r\n */;

	const char *s_end = s + len;
	int nread = 0;
	int cksum = 0;

	assert(s);
	assert(checksum);

	*checksum = -1;

	for (; s < s_end; s++, nread++) {
		if (('$' == *s) && nread) {
			s = NULL;
			break;
		}
		if ('*' == *s) {
			if (((s + tail_sz) <= s_end) && ('\r' == s[3]) && ('\n' == s[4])) {
				*checksum = nmea_atoi(s + 1, 2, 16);
				nread = len - (int) (s_end - (s + tail_sz));
				if (*checksum != cksum) {
					*checksum = -1;
					s = NULL;
				}
			}
			break;
		}
		if (nread) {
			cksum ^= (int) *s;
		}
	}

	if (s && (*checksum < 0))
		nread = 0;

	return nread;
}

/**
 * Parse a GPGGA sentence from a string
 *
 * @param s the string
 * @param len the length of the string
 * @param pack a pointer to the result structure
 * @return 1 (true) - if parsed successfully or 0 (false) otherwise.
 */
int nmea_parse_GPGGA(const char *s, int len, nmeaGPGGA *pack) {
	int token_count;
	char time_buff[NMEA_TIMEPARSE_BUF];
	size_t time_buff_len = 0;

	assert(s);
	assert(pack);

	nmea_trace_buff(s, len);

	/*
	 * Clear before parsing, to be able to detect absent fields
	 */
	time_buff[0] = '\0';
	pack->present = 0;
	pack->utc.hour = -1;
	pack->utc.min = -1;
	pack->utc.sec = -1;
	pack->utc.hsec = -1;
	pack->lat = NAN;
	pack->ns = 0;
	pack->lon = NAN;
	pack->ew = 0;
	pack->sig = -1;
	pack->satinuse = 0;
	pack->HDOP = NAN;
	pack->elv = NAN;
	pack->elv_units = 0;
	pack->diff = 0;			/* ignored */
	pack->diff_units = 0;	/* ignored */
	pack->dgps_age = 0;		/* ignored */
	pack->dgps_sid = 0;		/* ignored */

	/* parse */
	token_count = nmea_scanf(s, len, "$GPGGA,%s,%f,%C,%f,%C,%d,%d,%f,%f,%C,%f,%C,%f,%d*", &time_buff[0], &pack->lat,
			&pack->ns, &pack->lon, &pack->ew, &pack->sig, &pack->satinuse, &pack->HDOP, &pack->elv, &pack->elv_units,
			&pack->diff, &pack->diff_units, &pack->dgps_age, &pack->dgps_sid);

	/* see that we have enough tokens */
	if (token_count != 14) {
		nmea_error("GPGGA parse error!");
		return 0;
	}

	/* determine which fields are present and validate them */

	time_buff_len = strlen(&time_buff[0]);
	if (time_buff_len) {
		if (!_nmea_parse_time(&time_buff[0], time_buff_len, &pack->utc)) {
			return 0;
		}

		if (!validateTime(&pack->utc)) {
			return 0;
		}

		nmea_INFO_set_present(pack, UTCTIME);
	}
	if (pack->lat != NAN) {
		if (!pack->ns) {
			pack->ns = 'N';
		} else {
			if (!validateNSEW(&pack->ns, true)) {
				return 0;
			}

			/* only when lat and ns are present and valid */
			nmea_INFO_set_present(pack, LAT);
		}
	}
	if (pack->lon != NAN) {
		if (!pack->ew) {
			pack->ew = 'E';
		} else {
			if (!validateNSEW(&pack->ew, false)) {
				return 0;
			}

			/* only when lon and ew are present and valid */
			nmea_INFO_set_present(pack, LON);
		}
	}
	if (pack->sig != -1) {
		nmea_INFO_set_present(pack, SIG);
	}
	if (pack->HDOP != NAN) {
		nmea_INFO_set_present(pack, HDOP);
	}
	if (pack->elv != NAN) {
		if (!pack->elv_units) {
			pack->elv_units = 'M';
		} else {
			if (pack->elv_units != 'M') {
				nmea_error("Parse error: invalid elevation unit (%c)", pack->elv_units);
				return 0;
			}

			/* only when elv and elv_units are present and valid */
			nmea_INFO_set_present(pack, ELV);
		}
	}
	/* ignore diff and diff_units */
	/* ignore dgps_age and dgps_sid */

	return 1;
}

/**
 * Parse a GPGSA sentence from a string
 *
 * @param s the string
 * @param len the length of the string
 * @param pack a pointer to the result structure
 * @return 1 (true) - if parsed successfully or 0 (false) otherwise.
 */
int nmea_parse_GPGSA(const char *s, int len, nmeaGPGSA *pack) {
	int token_count;
	int i;

	assert(s);
	assert(pack);

	nmea_trace_buff(s, len);

	/*
	 * Clear before parsing, to be able to detect absent fields
	 */
	pack->present = 0;
	pack->fix_mode = 0;
	pack->fix_type = 0;
	for (i = 0; i < NMEA_MAXSAT; i++) {
		pack->sat_prn[i] = 0;
	}
	pack->PDOP = NAN;
	pack->HDOP = NAN;
	pack->VDOP = NAN;

	/* parse */
	token_count = nmea_scanf(s, len, "$GPGSA,%C,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f*", &pack->fix_mode,
			&pack->fix_type, &pack->sat_prn[0], &pack->sat_prn[1], &pack->sat_prn[2], &pack->sat_prn[3],
			&pack->sat_prn[4], &pack->sat_prn[5], &pack->sat_prn[6], &pack->sat_prn[7], &pack->sat_prn[8],
			&pack->sat_prn[9], &pack->sat_prn[10], &pack->sat_prn[11], &pack->PDOP, &pack->HDOP, &pack->VDOP);

	/* see that we have enough tokens */
	if (token_count != 17) {
		nmea_error("GPGSA parse error, need 17 tokens, got %d in %s", token_count, s);
		return 0;
	}

	/* determine which fields are present and validate them */

	pack->fix_mode = toupper(pack->fix_mode);
	if (!((pack->fix_mode == 'A') || (pack->fix_mode == 'M'))) {
		nmea_error("GPGSA parse error: invalid fix mode (%c)", pack->fix_mode);
		return 0;
	}
	if (!pack->fix_type) {
		nmea_INFO_set_present(pack, FIX);
	}
	for (i = 0; i < NMEA_MAXSAT; i++) {
		if (pack->sat_prn[i] != 0) {
			nmea_INFO_set_present(pack, SATINUSE);
			break;
		}
	}
	if (pack->PDOP != NAN) {
		nmea_INFO_set_present(pack, PDOP);
	}
	if (pack->HDOP != NAN) {
		nmea_INFO_set_present(pack, HDOP);
	}
	if (pack->VDOP != NAN) {
		nmea_INFO_set_present(pack, VDOP);
	}

	return 1;
}

/**
 * Parse a GPGSV sentence from a string
 *
 * @param s the string
 * @param len the length of the string
 * @param pack a pointer to the result structure
 * @return 1 (true) - if parsed successfully or 0 (false) otherwise.
 */
int nmea_parse_GPGSV(const char *s, int len, nmeaGPGSV *pack) {
	int token_count;
	int token_count_expected;
	int sat_count;
	int sat_counted = 0;

	assert(s);
	assert(pack);

	nmea_trace_buff(s, len);

	/*
	 * Clear before parsing, to be able to detect absent fields
	 */
	memset(pack, 0, sizeof(nmeaGPGSV));

	/* parse */
	token_count = nmea_scanf(s, len, "$GPGSV,%d,%d,%d,"
			"%d,%d,%d,%d,"
			"%d,%d,%d,%d,"
			"%d,%d,%d,%d,"
			"%d,%d,%d,%d*", &pack->pack_count, &pack->pack_index, &pack->sat_count, &pack->sat_data[0].id,
			&pack->sat_data[0].elv, &pack->sat_data[0].azimuth, &pack->sat_data[0].sig, &pack->sat_data[1].id,
			&pack->sat_data[1].elv, &pack->sat_data[1].azimuth, &pack->sat_data[1].sig, &pack->sat_data[2].id,
			&pack->sat_data[2].elv, &pack->sat_data[2].azimuth, &pack->sat_data[2].sig, &pack->sat_data[3].id,
			&pack->sat_data[3].elv, &pack->sat_data[3].azimuth, &pack->sat_data[3].sig);

	/* return if we have no sentences or sats */
	if ((pack->pack_count < 1) || (pack->pack_count > NMEA_NSATPACKS) || (pack->pack_index < 1)
			|| (pack->pack_index > pack->pack_count) || (pack->sat_count < 0) || (pack->sat_count > NMEA_MAXSAT)) {
		nmea_error("GPGSV parse error, inconsistent pack (count/index/satcount = %d/%d/%d)", pack->pack_count,
				pack->pack_index, pack->sat_count);
		return 0;
	}

	/* validate all sat settings and count the number of sats in the sentence */
	for (sat_count = 0; sat_count < NMEA_SATINPACK; sat_count++) {
		if (pack->sat_data[sat_count].id != 0) {
			if ((pack->sat_data[sat_count].id < 0)) {
				nmea_error("Parse error: invalid sat %d id (%d)", sat_count + 1, pack->sat_data[sat_count].id);
				return 0;
			}
			if ((pack->sat_data[sat_count].elv < 0) || (pack->sat_data[sat_count].elv > 90)) {
				nmea_error("Parse error: invalid sat %d elevation (%d)", sat_count + 1, pack->sat_data[sat_count].elv);
				return 0;
			}
			if ((pack->sat_data[sat_count].azimuth < 0) || (pack->sat_data[sat_count].azimuth >= 360)) {
				nmea_error("Parse error: invalid sat %d azimuth (%d)", sat_count + 1, pack->sat_data[sat_count].azimuth);
				return 0;
			}
			if ((pack->sat_data[sat_count].sig < 0) || (pack->sat_data[sat_count].sig > 99)) {
				nmea_error("Parse error: invalid sat %d signal (%d)", sat_count + 1, pack->sat_data[sat_count].sig);
				return 0;
			}
			sat_counted++;
		} else {
			memset(&pack->sat_data[sat_count], 0, sizeof(pack->sat_data[sat_count]));
		}
	}

	if (sat_counted != pack->sat_count) {
		nmea_error("GPGSV parse error, expected %d sats, got %d", pack->sat_count, sat_counted);
		return 0;
	}

	/* see that we have enough tokens */
	token_count_expected = (sat_counted * 4) + 3;
	if ((token_count < token_count_expected) || (token_count > (NMEA_SATINPACK * 4 + 3))) {
		nmea_error("GPGSV parse error, need %d tokens, got %d", token_count_expected, token_count);
		return 0;
	}

	/* determine which fields are present and validate them */

	if (pack->sat_count > 0) {
		nmea_INFO_set_present(pack, SATINVIEW);
	}

	return 1;
}

/**
 * Parse a GPRMC sentence from a string
 *
 * @param s the string
 * @param len the length of the string
 * @param pack a pointer to the result structure
 * @return 1 (true) - if parsed successfully or 0 (false) otherwise.
 */
int nmea_parse_GPRMC(const char *s, int len, nmeaGPRMC *pack) {
	int token_count;
	char time_buff[NMEA_TIMEPARSE_BUF];
	size_t time_buff_len = 0;

	assert(s);
	assert(pack);

	nmea_trace_buff(s, len);

	/*
	 * Clear before parsing, to be able to detect absent fields
	 */
	time_buff[0] = '\0';
	pack->present = 0;
	pack->utc.year = -1;
	pack->utc.mon = -1;
	pack->utc.day = -1;
	pack->utc.hour = -1;
	pack->utc.min = -1;
	pack->utc.sec = -1;
	pack->utc.hsec = -1;
	pack->status = 0;
	pack->lat = NAN;
	pack->ns = 0;
	pack->lon = NAN;
	pack->ew = 0;
	pack->speed = NAN;
	pack->track = NAN;
	pack->magvar = NAN;
	pack->magvar_ew = 0;
	pack->mode = 0;

	/* parse */
	token_count = nmea_scanf(s, len, "$GPRMC,%s,%C,%f,%C,%f,%C,%f,%f,%2d%2d%2d,%f,%C,%C*", &time_buff[0], &pack->status,
			&pack->lat, &pack->ns, &pack->lon, &pack->ew, &pack->speed, &pack->track, &pack->utc.day, &pack->utc.mon,
			&pack->utc.year, &pack->magvar, &pack->magvar_ew, &pack->mode);

	/* see that we have enough tokens */
	if ((token_count != 13) && (token_count != 14)) {
		nmea_error("GPRMC parse error, need 13 or 14 tokens, got %d in %s", token_count, s);
		return 0;
	}

	/* determine which fields are present and validate them */

	nmea_INFO_set_present(pack, SIG);
	nmea_INFO_set_present(pack, FIX);

	if ((pack->utc.year != -1) && (pack->utc.mon != -1) && (pack->utc.day != -1)) {
		if (pack->utc.year < 90) {
			pack->utc.year += 100;
		}
		pack->utc.mon -= 1;

		if (!validateDate(&pack->utc)) {
			return 0;
		}

		nmea_INFO_set_present(pack, UTCDATE);
	}

	time_buff_len = strlen(&time_buff[0]);
	if (time_buff_len) {
		if (!_nmea_parse_time(&time_buff[0], time_buff_len, &pack->utc)) {
			return 0;
		}

		if (!validateTime(&pack->utc)) {
			return 0;
		}

		nmea_INFO_set_present(pack, UTCTIME);
	}

	if (!pack->status) {
		pack->status = 'V';
	} else {
		pack->status = toupper(pack->status);
		if (!((pack->status == 'A') || (pack->status == 'V'))) {
			nmea_error("GPRMC parse error: invalid status (%c)", pack->status);
			return 0;
		}
	}
	if (pack->lat != NAN) {
		if (!pack->ns) {
			pack->ns = 'N';
		} else {
			if (!validateNSEW(&pack->ns, true)) {
				return 0;
			}

			/* only when lat and ns are present and valid */
			nmea_INFO_set_present(pack, LAT);
		}
	}
	if (pack->lon != NAN) {
		if (!pack->ew) {
			pack->ew = 'E';
		} else {
			if (!validateNSEW(&pack->ew, false)) {
				return 0;
			}

			/* only when lon and ew are present and valid */
			nmea_INFO_set_present(pack, LON);
		}
	}
	if (pack->speed != NAN) {
		nmea_INFO_set_present(pack, SPEED);
	}
	if (pack->track != NAN) {
		nmea_INFO_set_present(pack, TRACK);
	}
	if (pack->magvar != NAN) {
		if (!pack->magvar_ew) {
			pack->magvar_ew = 'E';
		} else {
			if (!validateNSEW(&pack->magvar_ew, false)) {
				return 0;
			}

			/* only when magvar and magvar_ew are present and valid */
			nmea_INFO_set_present(pack, MAGVAR);
		}
	}
	if (token_count == 13) {
		pack->mode = 'A';
	} else {
		if (!pack->mode) {
			pack->mode = 'N';
		} else {
			if (!validateMode(&pack->mode)) {
				return 0;
			}
		}
	}

	return 1;
}

/**
 * Parse a GPVTG sentence from a string
 *
 * @param s the string
 * @param len the length of the string
 * @param pack a pointer to the result structure
 * @return 1 (true) - if parsed successfully or 0 (false) otherwise.
 */
int nmea_parse_GPVTG(const char *s, int len, nmeaGPVTG *pack) {
	int token_count;

	assert(s);
	assert(pack);

	nmea_trace_buff(s, len);

	/*
	 * Clear before parsing, to be able to detect absent fields
	 */
	pack->present = 0;
	pack->track = NAN;
	pack->track_t = 0;
	pack->mtrack = NAN;
	pack->mtrack_m = 0;
	pack->spn = NAN;
	pack->spn_n = 0;
	pack->spk = NAN;
	pack->spk_k = 0;

	/* parse */
	token_count = nmea_scanf(s, len, "$GPVTG,%f,%C,%f,%C,%f,%C,%f,%C*", &pack->track, &pack->track_t, &pack->mtrack,
			&pack->mtrack_m, &pack->spn, &pack->spn_n, &pack->spk, &pack->spk_k);

	/* see that we have enough tokens */
	if (token_count != 8) {
		nmea_error("GPVTG parse error, need 8 tokens, got %d in %s", token_count, s);
		return 0;
	}

	/* uppercase the units */
	pack->track_t = toupper(pack->track_t);
	pack->mtrack_m = toupper(pack->mtrack_m);
	pack->spn_n = toupper(pack->spn_n);
	pack->spk_k = toupper(pack->spk_k);

	/* determine which fields are present and validate them */

	if (pack->track != NAN) {
		if (pack->track_t != 'T') {
			nmea_error("GPVTG parse error: invalid track unit, got %C, expected %C", pack->track_t, 'T');
			return 0;
		}

		nmea_INFO_set_present(pack, TRACK);
	}
	if (pack->mtrack != NAN) {
		if (pack->mtrack_m != 'M') {
			nmea_error("GPVTG parse error: invalid mtrack unit, got %C, expected %C", pack->mtrack_m, 'M');
			return 0;
		}

		nmea_INFO_set_present(pack, MTRACK);
	}
	if (pack->spn != NAN) {
		if (pack->spn_n != 'N') {
			nmea_error("GPVTG parse error: invalid knots speed unit, got %C, expected %C", pack->spn_n, 'N');
			return 0;
		}

		nmea_INFO_set_present(pack, SPEED);

		if (pack->spk == NAN) {
			pack->spk = pack->spn * NMEA_TUD_KNOTS;
			pack->spk_k = 'K';
		}
	}
	if (pack->spk != NAN) {
		if (pack->spk_k != 'K') {
			nmea_error("GPVTG parse error: invalid kph speed unit, got %C, expected %C", pack->spk_k, 'K');
			return 0;
		}

		nmea_INFO_set_present(pack, SPEED);

		if (pack->spn == NAN) {
			pack->spn = pack->spk / NMEA_TUD_KNOTS;
			pack->spn_n = 'N';
		}
	}

	return 1;
}
