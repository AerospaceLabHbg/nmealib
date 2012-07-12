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

/*! \file */

#ifndef __NMEA_INFO_H__
#define __NMEA_INFO_H__

#include <nmea/time.h>
#include <stdbool.h>

/*
 <pre>
 field/sentence GPGGA   GPGSA   GPGSV   GPRMC   GPVTG
 smask:         x       x       x       x       x
 utc:           x                       x
 sig:           x                       x
 fix:                   x               x
 PDOP:                  x
 HDOP:          x       x
 VDOP:                  x
 lat:           x                       x
 lon:           x                       x
 elv:           x
 speed:                                 x       x
 track:                                 x       x
 declination:                                   x
 satinfo:               x       x
 </pre>
 */

#define NMEA_SIG_BAD   (0)
#define NMEA_SIG_LOW   (1)
#define NMEA_SIG_MID   (2)
#define NMEA_SIG_HIGH  (3)

#define NMEA_FIX_BAD   (1)
#define NMEA_FIX_2D    (2)
#define NMEA_FIX_3D    (3)

#define NMEA_MAXSAT    (12)
#define NMEA_SATINPACK (4)
#define NMEA_NSATPACKS (NMEA_MAXSAT / NMEA_SATINPACK)

#define NMEA_DEF_LAT   (5001.2621)
#define NMEA_DEF_LON   (3613.0595)

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Position data in fractional degrees or radians
 */
typedef struct _nmeaPOS {
	double lat;						/**< Latitude */
	double lon;						/**< Longitude */
} nmeaPOS;

/**
 * Information about satellite
 * @see nmeaSATINFO
 * @see nmeaGPGSV
 */
typedef struct _nmeaSATELLITE {
	int id;							/**< Satellite PRN number */
	int in_use; 					/**< Used in position fix */
	int elv;						/**< Elevation in degrees, 90 maximum */
	int azimuth;					/**< Azimuth, degrees from true north, 000 to 359 */
	int sig;						/**< Signal, 00-99 dB */
} nmeaSATELLITE;

/**
 * Information about all satellites in view
 * @see nmeaINFO
 * @see nmeaGPGSV
 */
typedef struct _nmeaSATINFO {
	int inuse;						/**< Number of satellites in use (not those in view) */
	int inview;						/**< Total number of satellites in view */
	nmeaSATELLITE sat[NMEA_MAXSAT]; /**< Satellites information */
} nmeaSATINFO;

/**
 * Summary GPS information from all parsed packets,
 * used also for generating NMEA stream
 * @see nmea_parse
 * @see nmea_GPGGA2info,  nmea_...2info
 */
typedef struct _nmeaINFO {
	int present;					/**< Mask specifying which fields are present */

	int smask;						/**< Mask specifying from which sentences data has been obtained */

	nmeaTIME utc;					/**< UTC of position */

	int sig;						/**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
	int fix;						/**< Operating mode, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */

	double PDOP;					/**< Position Dilution Of Precision */
	double HDOP;					/**< Horizontal Dilution Of Precision */
	double VDOP;					/**< Vertical Dilution Of Precision */

	double lat;						/**< Latitude in NDEG - +/-[degree][min].[sec/60] */
	double lon;						/**< Longitude in NDEG - +/-[degree][min].[sec/60] */
	double elv;						/**< Antenna altitude above/below mean sea level (geoid) in meters */
	double speed;					/**< Speed over the ground in kilometers/hour */
	double track;					/**< Track angle in degrees True */
	double declination;				/**< Magnetic variation degrees (Easterly var. subtracts from true course) */

	nmeaSATINFO satinfo;			/**< Satellites information */
} nmeaINFO;

/**
 * Enumeration for the fields names of a nmeaINFO structure.
 * The values are used in the 'present' mask.
 */
typedef enum _nmeaINFO_FIELD {
	SMASK		= (1 << 0),
	UTC			= (1 << 1),
	SIG			= (1 << 2),
	FIX			= (1 << 3),
	PDOP		= (1 << 4),
	HDOP		= (1 << 5),
	VDOP		= (1 << 6),
	LAT			= (1 << 7),
	LON			= (1 << 8),
	ELV			= (1 << 9),
	SPEED		= (1 << 10),
	TRACK		= (1 << 11),
	DECLINATION	= (1 << 12),
	SATINFO		= (1 << 13)
} nmeaINFO_FIELD;

void nmea_zero_INFO(nmeaINFO *info);

/**
 * Determine if a nmeaINFO structure has a certain field
 *
 * @param nmeaInfo use a pointer to nmeaINFO
 * @param fieldName use a name from nmeaINFO_FIELD
 * @return a boolean, true when the structure has the requested field
 */
#define nmea_INFO_is_present(nmeaInfo, fieldName) (((nmeaInfo)->present & fieldName) != 0)

/**
 * Flag a nmeaINFO structure to contain a certain field
 *
 * @param nmeaInfo use a pointer to nmeaINFO
 * @param fieldName use a name from nmeaINFO_FIELD
 */
#define nmea_INFO_set_present(nmeaInfo, fieldName) ((nmeaInfo)->present |= fieldName)

/**
 * Flag a nmeaINFO structure to NOT contain a certain field
 *
 * @param nmeaInfo use a pointer to nmeaINFO
 * @param fieldName use a name from nmeaINFO_FIELD
 */
#define nmea_INFO_unset_present(nmeaInfo, fieldName) ((nmeaInfo)->present &= ~fieldName)

void nmea_INFO_sanitise(nmeaINFO *nmeaInfo);

void nmea_INFO_unit_conversion(nmeaINFO * nmeaInfo);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_INFO_H__ */
