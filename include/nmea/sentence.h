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
 * Extended descriptions of sentences are taken from
 *   http://www.gpsinformation.org/dale/nmea.htm
 */

#ifndef __NMEA_SENTENCE_H__
#define __NMEA_SENTENCE_H__

#include <nmea/info.h>

#include <stdint.h>
#include <stddef.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

static INLINE const char * nmea_INFO_smask_packtype_to_string(enum nmeaPACKTYPE packType) {
  switch (packType) {
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

/**
 * GGA packet information structure (Global Positioning System Fix Data)
 *
 * Essential fix data which provide 3D location and accuracy data.
 *
 * <pre>
 * $GPGGA,time,lat,ns,lon,ew,sig,sats,hdop,elv,elv unit,height,height unit,dgps age,dgps id*checksum
 * </pre>
 *
 * | Field       | Description                                            | present       |
 * | :---------: | ------------------------------------------------------ | :-----------: |
 * | $GPGGA      | NMEA prefix                                            | -             |
 * | time        | Fix time, in the format HHMMSS.hh (UTC)                | UTCTIME       |
 * | lat         | Latitude, in NDEG (DDMM.SSS)                           | LAT (1)       |
 * | ns          | North or south ('N' or 'S')                            | LAT (1)       |
 * | lon         | Longitude, in NDEG (DDDMM.SSS)                         | LON (2)       |
 * | ew          | East or west ('E' or 'W')                              | LON (2)       |
 * | sig         | Signal quality, see the NMEA_SIG_* defines             | SIG           |
 * | sats        | Number of satellites being tracked                     | SATINUSECOUNT |
 * | hdop        | Horizontal dilution of position                        | HDOP          |
 * | elv         | Altitude above mean sea level, in meters               | ELV (3)       |
 * | elv unit    | Unit of altitude ('M')                                 | ELV (3)       |
 * | height      | Height of geoid (mean sea level) above WGS84 ellipsoid | - (4)         |
 * | height unit | Unit of height ('M')                                   | - (4)         |
 * | dgps age    | Time since last DGPS update, in seconds                | - (4)         |
 * | dgps id     | DGPS station ID number                                 | - (4)         |
 * | checksum    | NMEA checksum                                          | -             |
 *
 * (1) These fields are both required for a valid latitude<br/>
 * (2) These fields are both required for a valid longitude<br/>
 * (3) These fields are both required for a valid altitude<br/>
 * (4) Not supported yet<br/>
 *
 * Example:
 *
 * <pre>
 *
 * $GPGGA,123519.43,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
 * </pre>
 *
 * Note that if the height of geoid is missing then the altitude should be
 * suspect. Some non-standard implementations report altitude with respect to
 * the ellipsoid rather than geoid altitude. Some units do not report negative
 * altitudes at all. This is the only sentence that reports altitude.
 */
typedef struct _nmeaGPGGA {
  uint32_t present;
  nmeaTIME time;
  double   lat;
  char     ns;
  double   lon;
  char     ew;
  int      sig;
  int      satinuse;
  double   HDOP;
  double   elv;
  char     elv_units;
  double   diff;
  char     diff_units;
  double   dgps_age;
  int      dgps_sid;
} nmeaGPGGA;

/**
 * GSA packet information structure (Satellite status)
 *
 * <pre>
 * GSA - GPS DOP and active satellites.
 *
 * This sentence provides details on the nature of the fix. It includes the
 * numbers of the satellites being used in the current solution and the DOP.
 *
 * DOP (dilution of precision) is an indication of the effect of satellite
 * geometry on the accuracy of the fix. It is a unitless number where smaller
 * is better. For 3D fixes using 4 satellites a 1.0 would be considered to be
 * a perfect number, however for overdetermined solutions it is possible to see
 * numbers below 1.0.
 *
 * There are differences in the way the PRN's are presented which can effect the
 * ability of some programs to display this data. For example, in the example
 * shown below there are 5 satellites in the solution and the null fields are
 * scattered indicating that the almanac would show satellites in the null
 * positions that are not being used as part of this solution. Other receivers
 * might output all of the satellites used at the beginning of the sentence with
 * the null field all stacked up at the end. This difference accounts for some
 * satellite display programs not always being able to display the satellites
 * being tracked. Some units may show all satellites that have ephemeris data
 * without regard to their use as part of the solution but this is non-standard.
 *
 * $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
 *
 * Where:
 *      GSA      Satellite status
 *      A        Auto selection of 2D or 3D fix (M = manual)
 *      3        3D fix - values include: 1 = no fix
 *                                        2 = 2D fix
 *                                        3 = 3D fix
 *      04,05... PRNs of satellites used for fix (space for 12)
 *      2.5      PDOP (dilution of precision)
 *      1.3      Horizontal dilution of precision (HDOP)
 *      2.1      Vertical dilution of precision (VDOP)
 *      *39      the checksum data, always begins with *
 * </pre>
 */
typedef struct _nmeaGPGSA {
    uint32_t present; /**< Mask specifying which fields are present, same as in nmeaINFO */
    char fix_mode; /**< Mode (M = Manual, forced to operate in 2D or 3D; A = Automatic, 3D/2D) */
    int fix_type; /**< Type, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */
    int sat_prn[NMEA_MAXSAT]; /**< PRNs of satellites used in position fix (0 for unused fields) */
    double PDOP; /**< Dilution of precision */
    double HDOP; /**< Horizontal dilution of precision */
    double VDOP; /**< Vertical dilution of precision */
} nmeaGPGSA;

/**
 * GSV packet information structure (Satellites in view)
 *
 * <pre>
 * GSV - Satellites in View
 *
 * Shows data about the satellites that the unit might be able to find based on
 * its viewing mask and almanac data. It also shows current ability to track
 * this data. Note that one GSV sentence only can provide data for up to 4
 * satellites and thus there may need to be 3 sentences for the full
 * information. It is reasonable for the GSV sentence to contain more satellites
 * than GGA might indicate since GSV may include satellites that are not used as
 * part of the solution. It is not a requirement that the GSV sentences all
 * appear in sequence. To avoid overloading the data bandwidth some receivers
 * may place the various sentences in totally different samples since each
 * sentence identifies which one it is.
 *
 * The field called SNR (Signal to Noise Ratio) in the NMEA standard is often
 * referred to as signal strength. SNR is an indirect but more useful value than
 * raw signal strength. It can range from 0 to 99 and has units of dB according
 * to the NMEA standard, but the various manufacturers send different ranges of
 * numbers with different starting numbers so the values themselves cannot
 * necessarily be used to evaluate different units. The range of working values
 * in a given gps will usually show a difference of about 25 to 35 between the
 * lowest and highest values, however 0 is a special case and may be shown on
 * satellites that are in view but not being tracked.
 *
 * $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75
 *
 * Where:
 *      GSV          Satellites in view
 *      2            Number of sentences for full data
 *      1            sentence 1 of 2
 *      08           Number of satellites in view
 *
 *      01           Satellite PRN number
 *      40           Elevation, degrees
 *      083          Azimuth, degrees
 *      46           SNR - higher is better
 *           for up to 4 satellites per sentence
 *
 *      *75          the checksum data, always begins with *
 * </pre>
 */
typedef struct _nmeaGPGSV {
    uint32_t present; /**< Mask specifying which fields are present, same as in nmeaINFO */
    int pack_count; /**< Total number of messages of this type in this cycle */
    int pack_index; /**< Message number */
    int sat_count; /**< Total number of satellites in view */
    nmeaSATELLITE sat_data[NMEA_SATINPACK];
} nmeaGPGSV;

/**
 * RMC -packet information structure (Recommended Minimum sentence C)
 *
 * <pre>
 * RMC - Recommended Minimum sentence C
 *
 * NMEA has its own version of essential gps pvt (position, velocity,
 * time) data. It is called RMC, the Recommended Minimum, which will look
 * similar to:
 *
 * $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
 * $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W,A*6A (v2.3)
 *
 * Where:
 *      RMC          Recommended Minimum sentence C
 *      123519       Fix taken at 12:35:19 UTC
 *      A            Status A=active or V=Void.
 *      4807.038,N   Latitude 48 deg 07.038' N
 *      01131.000,E  Longitude 11 deg 31.000' E
 *      022.4        Speed over the ground in knots
 *      084.4        Track angle in degrees True
 *      230394       Date - 23rd of March 1994
 *      003.1,W      Magnetic Variation
 *      A            Mode A=autonomous, D=differential, E=Estimated,
 *                        N=not valid, S=Simulator (NMEA v2.3)
 *      *6A          The checksum data, always begins with *
 * </pre>
 */
typedef struct _nmeaGPRMC {
    uint32_t present; /**< Mask specifying which fields are present, same as in nmeaINFO */
    nmeaTIME utc; /**< UTC of position */
    char status; /**< Status (A = active or V = void) */
    double lat; /**< Latitude in NDEG - [degree][min].[sec/60] */
    char ns; /**< [N]orth or [S]outh */
    double lon; /**< Longitude in NDEG - [degree][min].[sec/60] */
    char ew; /**< [E]ast or [W]est */
    double speed; /**< Speed over the ground in knots */
    double track; /**< Track angle in degrees True */
    double magvar; /**< Magnetic variation degrees (Easterly var. subtracts from true course) */
    char magvar_ew; /**< [E]ast or [W]est */
    char mode; /**< Mode indicator of fix type (A=autonomous, D=differential, E=Estimated, N=not valid, S=Simulator) */
} nmeaGPRMC;

/**
 * VTG packet information structure (Track made good and ground speed)
 *
 * <pre>
 * VTG - Velocity made good.
 *
 * The gps receiver may use the LC prefix instead of GP if it is emulating
 * Loran output.
 *
 * $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
 *
 * where:
 *      VTG          Track made good and ground speed
 *      054.7,T      True track made good (degrees)
 *      034.4,M      Magnetic track made good
 *      005.5,N      Ground speed, knots
 *      010.2,K      Ground speed, Kilometers per hour
 *      *48          Checksum
 * </pre>
 */
typedef struct _nmeaGPVTG {
    uint32_t present; /**< Mask specifying which fields are present, same as in nmeaINFO */
    double track; /**< True track made good (degrees) */
    char track_t; /**< Fixed text 'T' indicates that track made good is relative to true north */
    double mtrack; /**< Magnetic track made good */
    char mtrack_m; /**< Fixed text 'M' */
    double spn; /**< Ground speed, knots */
    char spn_n; /**< Fixed text 'N' indicates that speed over ground is in knots */
    double spk; /**< Ground speed, kilometers per hour */
    char spk_k; /**< Fixed text 'K' indicates that speed over ground is in kilometers/hour */
} nmeaGPVTG;

void nmea_zero_GPGSA(nmeaGPGSA *pack);
void nmea_zero_GPGSV(nmeaGPGSV *pack);
void nmea_zero_GPRMC(nmeaGPRMC *pack);
void nmea_zero_GPVTG(nmeaGPVTG *pack);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NMEA_SENTENCE_H__ */
