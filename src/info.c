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

#include <nmealib/info.h>

#include <nmealib/sentence.h>
#include <nmealib/util.h>
#include <math.h>
#include <nmealib/nmath.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

const char * nmeaInfoSigToString(NmeaSignal sig) {
  switch (sig) {
    case NMEALIB_SIG_INVALID:
      return "INVALID";

    case NMEALIB_SIG_FIX:
      return "FIX";

    case NMEALIB_SIG_DIFFERENTIAL:
      return "DIFFERENTIAL";

    case NMEALIB_SIG_SENSITIVE:
      return "SENSITIVE";

    case NMEALIB_SIG_RTKIN:
      return "REAL TIME KINEMATIC";

    case NMEALIB_SIG_FLOAT_RTK:
      return "FLOAT REAL TIME KINEMATIC";

    case NMEALIB_SIG_ESTIMATED:
      return "ESTIMATED (DEAD RECKONING)";

    case NMEALIB_SIG_MANUAL:
      return "MANUAL";

    case NMEALIB_SIG_SIMULATION:
      return "SIMULATION";

    default:
      return NULL;
  }
}

NmeaSignal nmeaInfoModeToSig(char mode) {
  switch (mode) {
    case 'N':
      return NMEALIB_SIG_INVALID;

    case 'A':
      return NMEALIB_SIG_FIX;

    case 'D':
      return NMEALIB_SIG_DIFFERENTIAL;

    case 'P':
      return NMEALIB_SIG_SENSITIVE;

    case 'R':
      return NMEALIB_SIG_RTKIN;

    case 'F':
      return NMEALIB_SIG_FLOAT_RTK;

    case 'E':
      return NMEALIB_SIG_ESTIMATED;

    case 'M':
      return NMEALIB_SIG_MANUAL;

    case 'S':
      return NMEALIB_SIG_SIMULATION;

    default:
      return NMEALIB_SIG_INVALID;
  }
}

char nmeaInfoSigToMode(NmeaSignal sig) {
  switch (sig) {
    case NMEALIB_SIG_INVALID:
      return 'N';

    case NMEALIB_SIG_FIX:
      return 'A';

    case NMEALIB_SIG_DIFFERENTIAL:
      return 'D';

    case NMEALIB_SIG_SENSITIVE:
      return 'P';

    case NMEALIB_SIG_RTKIN:
      return 'R';

    case NMEALIB_SIG_FLOAT_RTK:
      return 'F';

    case NMEALIB_SIG_ESTIMATED:
      return 'E';

    case NMEALIB_SIG_MANUAL:
      return 'M';

    case NMEALIB_SIG_SIMULATION:
      return 'S';

    default:
      return 'N';
  }
}

const char * nmeaInfoFixToString(NmeaFix fix) {
  switch (fix) {
    case NMEALIB_FIX_BAD:
      return "BAD";

    case NMEALIB_FIX_2D:
      return "2D";

    case NMEALIB_FIX_3D:
      return "3D";

    default:
      return NULL;
  }
}

const char * nmeaInfoFieldToString(NmeaPresence field) {
  switch (field) {
    case NMEALIB_PRESENT_SMASK:
      return "SMASK";

    case NMEALIB_PRESENT_UTCDATE:
      return "UTCDATE";

    case NMEALIB_PRESENT_UTCTIME:
      return "UTCTIME";

    case NMEALIB_PRESENT_SIG:
      return "SIG";

    case NMEALIB_PRESENT_FIX:
      return "FIX";

    case NMEALIB_PRESENT_PDOP:
      return "PDOP";

    case NMEALIB_PRESENT_HDOP:
      return "HDOP";

    case NMEALIB_PRESENT_VDOP:
      return "VDOP";

    case NMEALIB_PRESENT_LAT:
      return "LAT";

    case NMEALIB_PRESENT_LON:
      return "LON";

    case NMEALIB_PRESENT_ELV:
      return "ELV";

    case NMEALIB_PRESENT_HEIGHT:
      return "HEIGHT";

    case NMEALIB_PRESENT_SPEED:
      return "SPEED";

    case NMEALIB_PRESENT_TRACK:
      return "TRACK";

    case NMEALIB_PRESENT_MTRACK:
      return "MTRACK";

    case NMEALIB_PRESENT_MAGVAR:
      return "MAGVAR";

    case NMEALIB_PRESENT_SATINUSECOUNT:
      return "SATINUSECOUNT";

    case NMEALIB_PRESENT_SATINUSE:
      return "SATINUSE";

    case NMEALIB_PRESENT_SATINVIEWCOUNT:
      return "SATINVIEWCOUNT";

    case NMEALIB_PRESENT_SATINVIEW:
      return "SATINVIEW";

    case NMEALIB_PRESENT_DGPSAGE:
      return "DGPSAGE";

    case NMEALIB_PRESENT_DGPSSID:
      return "DGPSSID";

    default:
      return NULL;
  }
}

bool nmeaTimeParseTime(const char *s, NmeaTime *time) {
  const char *tm;
  size_t sz;

  if (!s //
      || !time) {
    return false;
  }

  tm = s;
  sz = nmeaStringTrim(&tm);

  if (nmeaStringContainsWhitespace(tm, sz)) {
    return false;
  }

  if (sz == 6) { // HHMMSS
    time->hsec = 0;
    return (3 == nmeaScanf(tm, sz, "%2u%2u%2u", &time->hour, &time->min, &time->sec));
  }

  if (sz == 8) { // HHMMSS.t
    if (4 == nmeaScanf(tm, sz, "%2u%2u%2u.%u", &time->hour, &time->min, &time->sec, &time->hsec)) {
      time->hsec *= 10;
      return true;
    }
    return false;
  }

  if (sz == 9) { // HHMMSS.hh
    return (4 == nmeaScanf(tm, sz, "%2u%2u%2u.%u", &time->hour, &time->min, &time->sec, &time->hsec));
  }

  if (sz == 10) { // HHMMSS.mmm
    if ((4 == nmeaScanf(tm, sz, "%2u%2u%2u.%u", &time->hour, &time->min, &time->sec, &time->hsec))) {
      time->hsec = (time->hsec + 5) / 10;
      return true;
    }
    return false;
  }

  return false;
}

bool nmeaTimeParseDate(const char *s, NmeaTime *date) {
  size_t sz;
  const char * d;

  if (!s //
      || !date) {
    return false;
  }

  d = s;
  sz = nmeaStringTrim(&d);

  if (nmeaStringContainsWhitespace(d, sz)) {
    return false;
  }

  if (sz != 6) {
    return false;
  }

  if (3 != nmeaScanf(d, sz, "%2u%2u%2u", &date->day, &date->mon, &date->year)) {
    return false;
  }

  if (date->year > 90) {
    date->year += 1900;
  } else {
    date->year += 2000;
  }

  return true;
}

void nmeaTimeSet(NmeaTime *utc, uint32_t * present, struct timeval *timeval) {
  struct timeval tp;
  struct tm tt;
  long usec;

  if (!utc) {
    return;
  }

  if (timeval) {
    gmtime_r(&timeval->tv_sec, &tt);
    usec = timeval->tv_usec;
  } else {
    gettimeofday(&tp, NULL);
    gmtime_r(&tp.tv_sec, &tt);
    usec = tp.tv_usec;
  }


  utc->year = (unsigned int) tt.tm_year + 1900;
  utc->mon = (unsigned int) tt.tm_mon + 1;
  utc->day = (unsigned int) tt.tm_mday;
  utc->hour = (unsigned int) tt.tm_hour;
  utc->min = (unsigned int) tt.tm_min;
  utc->sec = (unsigned int) tt.tm_sec;
  utc->hsec = (unsigned int) (usec / 10000);
  if (present) {
    nmeaInfoSetPresent(present, NMEALIB_PRESENT_UTCDATE | NMEALIB_PRESENT_UTCTIME);
  }
}

void nmeaInfoClear(NmeaInfo *info) {
  if (!info) {
    return;
  }

  memset(info, 0, sizeof(NmeaInfo));

  info->sig = NMEALIB_SIG_INVALID;
  nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_SIG);

  info->fix = NMEALIB_FIX_BAD;
  nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_FIX);
}

void nmeaInfoSanitise(NmeaInfo *info) {
  double lat = 0;
  double lon = 0;
  double speed = 0;
  double track = 0;
  double mtrack = 0;
  double magvar = 0;
  bool latAdjusted = false;
  bool lonAdjusted = false;
  bool speedAdjusted = false;
  bool trackAdjusted = false;
  bool mtrackAdjusted = false;
  bool magvarAdjusted = false;
  NmeaTime utc;
  size_t i;

  if (!info) {
    return;
  }

  /* convert back to non-metric */
  nmeaInfoUnitConversion(info, false);

  /*
   * Reset to default when not present
   */

  /* no need to reset present; always present */

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SMASK)) {
    info->smask = 0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_UTCDATE | NMEALIB_PRESENT_UTCTIME)) {
    nmeaTimeSet(&utc, NULL, NULL);
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_UTCDATE)) {
    info->utc.year = utc.year;
    info->utc.mon = utc.mon;
    info->utc.day = utc.day;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_UTCTIME)) {
    info->utc.hour = utc.hour;
    info->utc.min = utc.min;
    info->utc.sec = utc.sec;
    info->utc.hsec = utc.hsec;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SIG)) {
    info->sig = NMEALIB_SIG_INVALID;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_FIX)) {
    info->fix = NMEALIB_FIX_BAD;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_PDOP)) {
    info->pdop = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_HDOP)) {
    info->hdop = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_VDOP)) {
    info->vdop = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LAT)) {
    info->lat = NMEALIB_LATITUDE_DEFAULT_NDEG;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LON)) {
    info->lon = NMEALIB_LONGITUDE_DEFAULT_NDEG;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_ELV)) {
    info->elv = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_HEIGHT)) {
    info->height = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SPEED)) {
    info->speed = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_TRACK)) {
    info->track = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_MTRACK)) {
    info->mtrack = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_MAGVAR)) {
    info->magvar = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_DGPSAGE)) {
    info->dgpsAge = 0.0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_DGPSSID)) {
    info->dgpsSid = 0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINUSECOUNT)) {
    info->satinfo.inUseCount = 0;
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINUSE)) {
    memset(&info->satinfo.inUse, 0, sizeof(info->satinfo.inUse));
  }

  if (!nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINVIEWCOUNT)) {
    info->satinfo.inViewCount = 0;
  }

  if (!info->progress.gpgsvInProgress //
      && !nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINVIEW)) {
    memset(&info->satinfo.inView, 0, sizeof(info->satinfo.inView));
  }

  /*
   * present
   */

  info->present = info->present & NMEALIB_INFO_PRESENT_MASK;

  /*
   * smask
   */

  info->smask = info->smask & NMEALIB_SENTENCE_MASK;

  /*
   * utc
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_UTCDATE)) {
    if (info->utc.year < 1990) {
      info->utc.year = 1990;
    } else if (info->utc.year > 2189) {
      info->utc.year = 2189;
    }
    if (info->utc.mon < 1) {
      info->utc.mon = 1;
    } else if (info->utc.mon > 12) {
      info->utc.mon = 12;
    }
    if (info->utc.day < 1) {
      info->utc.day = 1;
    } else if (info->utc.day > 31) {
      info->utc.day = 31;
    }
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_UTCDATE)) {
    info->utc.hour = info->utc.hour %  24;
    info->utc.min  = info->utc.min  %  60;
    info->utc.sec  = info->utc.sec  %  61;
    info->utc.hsec = info->utc.hsec % 100;
  }

  /*
   * sig
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SIG)) {
    if ((info->sig < NMEALIB_SIG_FIRST) //
        || (info->sig > NMEALIB_SIG_LAST)) {
      info->sig = NMEALIB_SIG_INVALID;
    }
  }

  /*
   * fix
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_FIX)) {
    if ((info->fix < NMEALIB_FIX_FIRST) //
        || (info->fix > NMEALIB_FIX_LAST)) {
      info->fix = NMEALIB_FIX_BAD;
    }
  }

  /*
   * pdop
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_PDOP)) {
    info->pdop = fabs(info->pdop);
  }

  /*
   * hdop
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_HDOP)) {
    info->hdop = fabs(info->hdop);
  }

  /*
   * vdop
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_VDOP)) {
    info->vdop = fabs(info->vdop);
  }

  /*
   * lat
   */

  lat = info->lat;
  lon = info->lon;

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LAT)) {
    /* force lat in [-18000, 18000] */
    while (lat < -18000.0) {
      lat += 36000.0;
      latAdjusted = true;
    }
    while (lat > 18000.0) {
      lat -= 36000.0;
      latAdjusted = true;
    }

    /* lat is now in [-18000, 18000] */

    /* force lat from <9000, 18000] in [9000, 0] */
    if (lat > 9000.0) {
      lat = 18000.0 - lat;
      lon += 18000.0;
      latAdjusted = true;
      lonAdjusted = true;
    }

    /* force lat from [-18000, -9000> in [0, -9000] */
    if (lat < -9000.0) {
      lat = -18000.0 - lat;
      lon += 18000.0;
      latAdjusted = true;
      lonAdjusted = true;
    }

    /* lat is now in [-9000, 9000] */

    if (latAdjusted) {
      info->lat = lat;
    }
  }

  /*
   * lon
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LON)) {
    /* force lon in [-18000, 18000] */
    while (lon < -18000.0) {
      lon += 36000.0;
      lonAdjusted = true;
    }
    while (lon > 18000.0) {
      lon -= 36000.0;
      lonAdjusted = true;
    }

    /* lon is now in [-18000, 18000] */

    if (lonAdjusted) {
      info->lon = lon;
    }
  }

  /*
   * elv
   */

  /*
   * height
   */

  /*
   * speed
   */

  speed = info->speed;
  track = info->track;
  mtrack = info->mtrack;

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SPEED)) {
    if (speed < 0.0) {
      speed = -speed;
      track += 180.0;
      mtrack += 180.0;
      speedAdjusted = true;
      trackAdjusted = true;
      mtrackAdjusted = true;
    }

    /* speed is now in [0, max> */

    if (speedAdjusted) {
      info->speed = speed;
    }
  }

  /*
   * track
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_TRACK)) {
    /* force track in [0, 360> */
    while (track < 0.0) {
      track += 360.0;
      trackAdjusted = true;
    }
    while (track >= 360.0) {
      track -= 360.0;
      trackAdjusted = true;
    }

    /* track is now in [0, 360> */

    if (trackAdjusted) {
      info->track = track;
    }
  }

  /*
   * mtrack
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_MTRACK)) {
    /* force mtrack in [0, 360> */
    while (mtrack < 0.0) {
      mtrack += 360.0;
      mtrackAdjusted = true;
    }
    while (mtrack >= 360.0) {
      mtrack -= 360.0;
      mtrackAdjusted = true;
    }

    /* mtrack is now in [0, 360> */

    if (mtrackAdjusted) {
      info->mtrack = mtrack;
    }
  }

  /*
   * magvar
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_MAGVAR)) {
    info->magvar = fabs(info->magvar);

    magvar = info->magvar;

    /* force magvar in [0, 360> */
    while (magvar < 0.0) {
      magvar += 360.0;
      magvarAdjusted = true;
    }
    while (magvar >= 360.0) {
      magvar -= 360.0;
      magvarAdjusted = true;
    }

    /* magvar is now in [0, 360> */

    if (magvarAdjusted) {
      info->magvar = magvar;
    }
  }

  /*
   * dgpsAge
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_DGPSAGE)) {
    info->dgpsAge = fabs(info->dgpsAge);
  }

  /*
   * dgpsSid
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_DGPSSID)) {
    info->dgpsSid = abs(info->dgpsSid);
  }

  /*
   * satinfo
   */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINUSECOUNT)) {
    info->satinfo.inUseCount = abs(info->satinfo.inUseCount);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINUSE)) {
    qsort(info->satinfo.inUse, NMEALIB_MAX_SATELLITES, sizeof(info->satinfo.inUse[0]), nmeaQsortPRNCompact);

    for (i = 0; i < NMEALIB_MAX_SATELLITES; i++) {
      if (!info->satinfo.inUse[i]) {
        break;
      }

      info->satinfo.inUse[i] = abs(info->satinfo.inUse[i]);
    }
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINVIEWCOUNT)) {
    info->satinfo.inViewCount = abs(info->satinfo.inViewCount);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINVIEW) //
      && !info->progress.gpgsvInProgress) {
    qsort(info->satinfo.inView, NMEALIB_MAX_SATELLITES, sizeof(info->satinfo.inView[0]), nmeaQsortSatelliteCompact);

    for (i = 0; i < NMEALIB_MAX_SATELLITES; i++) {
      NmeaSatellite *sat = &info->satinfo.inView[i];
      if (!sat->prn) {
        break;
      }

      sat->prn = abs(sat->prn);

      /* force elv in [-180, 180] */
      while (sat->elevation < -180) {
        sat->elevation += 360;
      }
      while (sat->elevation > 180) {
        sat->elevation -= 360;
      }

      /* elv is now in [-180, 180] */

      /* force elv from <90, 180] in [90, 0] */
      if (sat->elevation > 90) {
        sat->elevation = 180 - sat->elevation;
        sat->azimuth += 180;
      }

      /* force elv from [-180, -90> in [0, -90] */
      if (sat->elevation < -90) {
        sat->elevation = -180 - sat->elevation;
        sat->azimuth += 180;
      }

      /* elv is now in [-90, 90] */

      /* force azimuth in [0, 360> */
      while (sat->azimuth < 0) {
        sat->azimuth += 360;
      }
      while (sat->azimuth >= 360) {
        sat->azimuth -= 360;
      }
      /* azimuth is now in [0, 360> */

      /* force snr in [0, 99] */
      if (sat->snr < 0) {
        sat->snr = 0;
      }
      if (sat->snr > 99) {
        sat->snr = 99;
      }
      /* snr is now in [0, 99] */
    }
  }
}

void nmeaInfoUnitConversion(NmeaInfo * info, bool toMetric) {
  if (!info) {
    return;
  }

  if (info->metric == toMetric) {
    return;
  }

  /* smask (already in correct format) */

  /* utc (already in correct format) */

  /* sig (already in correct format) */
  /* fix (already in correct format) */

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_PDOP)) {
    if (toMetric) {
      info->pdop = nmeaMathDopToMeters(info->pdop);
    } else {
      info->pdop = nmeaMathMetersToDop(info->pdop);
    }
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_HDOP)) {
    if (toMetric) {
      info->hdop = nmeaMathDopToMeters(info->hdop);
    } else {
      info->hdop = nmeaMathMetersToDop(info->hdop);
    }
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_VDOP)) {
    if (toMetric) {
      info->vdop = nmeaMathDopToMeters(info->vdop);
    } else {
      info->vdop = nmeaMathMetersToDop(info->vdop);
    }
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LAT)) {
    if (toMetric) {
      info->lat = nmeaMathNdegToDegree(info->lat);
    } else {
      info->lat = nmeaMathDegreeToNdeg(info->lat);
    }
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_LON)) {
    if (toMetric) {
      info->lon = nmeaMathNdegToDegree(info->lon);
    } else {
      info->lon = nmeaMathDegreeToNdeg(info->lon);
    }
  }

  /* elv (already in correct format) */
  /* speed (already in correct format) */
  /* track (already in correct format) */
  /* mtrack (already in correct format) */
  /* magvar (already in correct format) */

  /* satinfo (already in correct format) */

  info->metric = toMetric;
}

int nmeaQsortPRNCompare(const void *p1, const void *p2) {
  int prn1 = *((const int *) p1);
  int prn2 = *((const int *) p2);

  if (!prn1) {
    prn1 += 1000;
  }
  if (!prn2) {
    prn2 += 1000;
  }

  return (prn1 - prn2);
}

int nmeaQsortPRNCompact(const void *p1, const void *p2) {
  int prn1 = *((const int *) p1);
  int prn2 = *((const int *) p2);

  if (prn1 && prn2) {
    return 0;
  }

  return nmeaQsortPRNCompare(p1, p2);
}

int nmeaQsortSatelliteCompare(const void *s1, const void *s2) {
  const NmeaSatellite *sat1 = (const NmeaSatellite *) s1;
  const NmeaSatellite *sat2 = (const NmeaSatellite *) s2;

  return nmeaQsortPRNCompare(&sat1->prn, &sat2->prn);
}

int nmeaQsortSatelliteCompact(const void *s1, const void *s2) {
  const NmeaSatellite *sat1 = (const NmeaSatellite *) s1;
  const NmeaSatellite *sat2 = (const NmeaSatellite *) s2;

  return nmeaQsortPRNCompact(&sat1->prn, &sat2->prn);
}
