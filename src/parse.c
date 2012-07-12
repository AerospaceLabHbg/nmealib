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

#include <nmea/time.h>
#include <nmea/tok.h>
#include <nmea/context.h>
#include <nmea/units.h>

#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

#define NMEA_TIMEPARSE_BUF  (256)

/**
 * Parse nmeaTIME (time only, no date) from a string.
 * The format that is used (hhmmss, hhmmss.s, hhmmss.ss or hhmmss.sss) is determined by the length of the string.
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
		nmea_error("Parse error: invalid time (%02d%02d%02d.%02d)", t->hour, t->min, t->sec, t->hsec);
		return false;
	}

	return true;
}

/**
 * Validate the date fields in an nmeaTIME structure.
 * Expects:
 * <pre>
 *   90 <= year  < 190
 *    0 <= month <  12
 *    0 <= day   <  31
 * </pre>
 *
 * @param t a pointer to the structure
 * @return true when valid, false otherwise
 */
static bool validateDate(nmeaTIME * t) {
	if (!t) {
		return false;
	}

	if (!((t->year >= 90) && (t->year < 190) && (t->mon >= 0) && (t->mon < 12) && (t->day >= 0) && (t->day < 31))) {
		nmea_error("Parse error: invalid date (%02d%02d%02d)", t->day, t->mon, t->year);
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

	for (; s < s_end; ++s, ++nread) {
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
 * \brief Parse GGA packet from buffer.
 * @param s a constant character pointer of packet buffer.
 * @param len buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPGGA(const char *s, int len, nmeaGPGGA *pack) {
	char time_buff[NMEA_TIMEPARSE_BUF];

	assert(s && pack);

	memset(pack, 0, sizeof(nmeaGPGGA));

	nmea_trace_buff(s, len);

	if (14
			!= nmea_scanf(s, len, "$GPGGA,%s,%f,%C,%f,%C,%d,%d,%f,%f,%C,%f,%C,%f,%d*", &(time_buff[0]),
					&(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew), &(pack->sig), &(pack->satinuse),
					&(pack->HDOP), &(pack->elv), &(pack->elv_units), &(pack->diff), &(pack->diff_units),
					&(pack->dgps_age), &(pack->dgps_sid))) {
		nmea_error("GPGGA parse error!");
		return 0;
	}

	if (!_nmea_parse_time(&time_buff[0], (int) strlen(&time_buff[0]), &(pack->utc))) {
		nmea_error("GPGGA time parse error!");
		return 0;
	}

	return 1;
}

/**
 * \brief Parse GSA packet from buffer.
 * @param s a constant character pointer of packet buffer.
 * @param len buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPGSA(const char *s, int len, nmeaGPGSA *pack) {
	assert(s && pack);

	memset(pack, 0, sizeof(nmeaGPGSA));

	nmea_trace_buff(s, len);

	if (17
			!= nmea_scanf(s, len, "$GPGSA,%C,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f*",
					&(pack->fix_mode), &(pack->fix_type), &(pack->sat_prn[0]), &(pack->sat_prn[1]), &(pack->sat_prn[2]),
					&(pack->sat_prn[3]), &(pack->sat_prn[4]), &(pack->sat_prn[5]), &(pack->sat_prn[6]),
					&(pack->sat_prn[7]), &(pack->sat_prn[8]), &(pack->sat_prn[9]), &(pack->sat_prn[10]),
					&(pack->sat_prn[11]), &(pack->PDOP), &(pack->HDOP), &(pack->VDOP))) {
		nmea_error("GPGSA parse error!");
		return 0;
	}

	return 1;
}

/**
 * \brief Parse GSV packet from buffer.
 * @param s a constant character pointer of packet buffer.
 * @param len buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPGSV(const char *s, int len, nmeaGPGSV *pack) {
	int nsen, nsat;

	assert(s && pack);

	memset(pack, 0, sizeof(nmeaGPGSV));

	nmea_trace_buff(s, len);

	nsen = nmea_scanf(s, len, "$GPGSV,%d,%d,%d,"
			"%d,%d,%d,%d,"
			"%d,%d,%d,%d,"
			"%d,%d,%d,%d,"
			"%d,%d,%d,%d*", &(pack->pack_count), &(pack->pack_index), &(pack->sat_count), &(pack->sat_data[0].id),
			&(pack->sat_data[0].elv), &(pack->sat_data[0].azimuth), &(pack->sat_data[0].sig), &(pack->sat_data[1].id),
			&(pack->sat_data[1].elv), &(pack->sat_data[1].azimuth), &(pack->sat_data[1].sig), &(pack->sat_data[2].id),
			&(pack->sat_data[2].elv), &(pack->sat_data[2].azimuth), &(pack->sat_data[2].sig), &(pack->sat_data[3].id),
			&(pack->sat_data[3].elv), &(pack->sat_data[3].azimuth), &(pack->sat_data[3].sig));

	nsat = (pack->pack_index - 1) * NMEA_SATINPACK;
	nsat = (nsat + NMEA_SATINPACK > pack->sat_count) ? pack->sat_count - nsat : NMEA_SATINPACK;
	nsat = nsat * 4 + 3 /* first three sentence`s */;

	if (nsen < nsat || nsen > (NMEA_SATINPACK * 4 + 3)) {
		nmea_error("GPGSV parse error!");
		return 0;
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
	int nsen;
	char time_buff[NMEA_TIMEPARSE_BUF];

	assert(s && pack);

	memset(pack, 0, sizeof(nmeaGPRMC));

	nmea_trace_buff(s, len);

	nsen = nmea_scanf(s, len, "$GPRMC,%s,%C,%f,%C,%f,%C,%f,%f,%2d%2d%2d,%f,%C,%C*", &(time_buff[0]),
			&(pack->status), &(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew), &(pack->speed), &(pack->track),
			&(pack->utc.day), &(pack->utc.mon), &(pack->utc.year), &(pack->magvar), &(pack->magvar_ew),
			&(pack->mode));

	if (nsen != 13 && nsen != 14) {
		nmea_error("GPRMC parse error!");
		return 0;
	}

	if (!_nmea_parse_time(&time_buff[0], (int) strlen(&time_buff[0]), &(pack->utc))) {
		nmea_error("GPRMC time parse error!");
		return 0;
	}

	if (pack->utc.year < 90)
		pack->utc.year += 100;
	pack->utc.mon -= 1;

	return 1;
}

/**
 * \brief Parse VTG packet from buffer.
 * @param s a constant character pointer of packet buffer.
 * @param len buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPVTG(const char *s, int len, nmeaGPVTG *pack) {
	assert(s && pack);

	memset(pack, 0, sizeof(nmeaGPVTG));

	nmea_trace_buff(s, len);

	if (8
			!= nmea_scanf(s, len, "$GPVTG,%f,%C,%f,%C,%f,%C,%f,%C*", &(pack->dir), &(pack->dir_t), &(pack->dec),
					&(pack->dec_m), &(pack->spn), &(pack->spn_n), &(pack->spk), &(pack->spk_k))) {
		nmea_error("GPVTG parse error!");
		return 0;
	}

	if (pack->dir_t != 'T' || pack->dec_m != 'M' || pack->spn_n != 'N' || pack->spk_k != 'K') {
		nmea_error("GPVTG parse error (format error)!");
		return 0;
	}

	return 1;
}

/**
 * \brief Fill nmeaINFO structure by GGA packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPGGA2info(nmeaGPGGA *pack, nmeaINFO *info) {
	assert(pack && info);

	info->utc.hour = pack->utc.hour;
	info->utc.min = pack->utc.min;
	info->utc.sec = pack->utc.sec;
	info->utc.hsec = pack->utc.hsec;
	info->sig = pack->sig;
	info->HDOP = pack->HDOP;
	info->elv = pack->elv;
	info->lat = ((pack->ns == 'N') ? pack->lat : -(pack->lat));
	info->lon = ((pack->ew == 'E') ? pack->lon : -(pack->lon));
	info->smask |= GPGGA;
}

/**
 * \brief Fill nmeaINFO structure by GSA packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPGSA2info(nmeaGPGSA *pack, nmeaINFO *info) {
	int i, j, nuse = 0;

	assert(pack && info);

	info->fix = pack->fix_type;
	info->PDOP = pack->PDOP;
	info->HDOP = pack->HDOP;
	info->VDOP = pack->VDOP;

	for (i = 0; i < NMEA_MAXSAT; ++i) {
		for (j = 0; j < info->satinfo.inview; ++j) {
			if (pack->sat_prn[i] && pack->sat_prn[i] == info->satinfo.sat[j].id) {
				info->satinfo.sat[j].in_use = 1;
				nuse++;
			}
		}
	}

	info->satinfo.inuse = nuse;
	info->smask |= GPGSA;
}

/**
 * \brief Fill nmeaINFO structure by GSV packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPGSV2info(nmeaGPGSV *pack, nmeaINFO *info) {
	int isat, isi, nsat;

	assert(pack && info);

	if (pack->pack_index > pack->pack_count || pack->pack_index * NMEA_SATINPACK > NMEA_MAXSAT)
		return;

	if (pack->pack_index < 1)
		pack->pack_index = 1;

	info->satinfo.inview = pack->sat_count;

	nsat = (pack->pack_index - 1) * NMEA_SATINPACK;
	nsat = (nsat + NMEA_SATINPACK > pack->sat_count) ? pack->sat_count - nsat : NMEA_SATINPACK;

	for (isat = 0; isat < nsat; ++isat) {
		isi = (pack->pack_index - 1) * NMEA_SATINPACK + isat;
		info->satinfo.sat[isi].id = pack->sat_data[isat].id;
		info->satinfo.sat[isi].elv = pack->sat_data[isat].elv;
		info->satinfo.sat[isi].azimuth = pack->sat_data[isat].azimuth;
		info->satinfo.sat[isi].sig = pack->sat_data[isat].sig;
	}

	info->smask |= GPGSV;
}

/**
 * Fill nmeaINFO structure from RMC packet structure
 *
 * @param pack a pointer to the packet structure
 * @param info a pointer to the nmeaINFO structure
 */
void nmea_GPRMC2info(nmeaGPRMC *pack, nmeaINFO *info) {
	assert(pack && info);

	if ('A' == pack->status) {
		if (NMEA_SIG_BAD == info->sig)
			info->sig = NMEA_SIG_MID;
		if (NMEA_FIX_BAD == info->fix)
			info->fix = NMEA_FIX_2D;
	} else if ('V' == pack->status) {
		info->sig = NMEA_SIG_BAD;
		info->fix = NMEA_FIX_BAD;
	}

	info->utc = pack->utc;
	info->lat = ((pack->ns == 'N') ? pack->lat : -(pack->lat));
	info->lon = ((pack->ew == 'E') ? pack->lon : -(pack->lon));
	info->speed = pack->speed * NMEA_TUD_KNOTS;
	info->track = pack->track;
	info->smask |= GPRMC;
}

/**
 * \brief Fill nmeaINFO structure by VTG packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPVTG2info(nmeaGPVTG *pack, nmeaINFO *info) {
	assert(pack && info);

	info->track = pack->dir;
	info->magvar = pack->dec;
	info->speed = pack->spk;
	info->smask |= GPVTG;
}
