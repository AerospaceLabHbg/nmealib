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

#include <nmealib/gpgsa.h>

#include <nmealib/context.h>
#include <nmealib/sentence.h>
#include <nmealib/validate.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool nmeaGPGSAParse(const char *s, const size_t sz, NmeaGPGSA *pack) {
  size_t fieldCount;

  if (!s //
      || !sz //
      || !pack) {
    return false;
  }

  nmeaContextTraceBuffer(s, sz);

  /* Clear before parsing, to be able to detect absent fields */
  memset(pack, 0, sizeof(*pack));
  pack->fix = UINT_MAX;
  pack->pdop = NAN;
  pack->hdop = NAN;
  pack->vdop = NAN;

  /* parse */
  fieldCount = nmeaScanf(s, sz, //
      "$" NMEALIB_GPGSA_PREFIX ",%C,%d," //
      "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"//
      "%F,%F,%F*",//
      &pack->sig, //
      &pack->fix, //
      &pack->prn[0], //
      &pack->prn[1], //
      &pack->prn[2], //
      &pack->prn[3], //
      &pack->prn[4], //
      &pack->prn[5], //
      &pack->prn[6], //
      &pack->prn[7], //
      &pack->prn[8], //
      &pack->prn[9], //
      &pack->prn[10], //
      &pack->prn[11], //
      &pack->pdop, //
      &pack->hdop, //
      &pack->vdop);

  /* see that there are enough tokens */
  if (fieldCount != 17) {
    nmeaContextError(NMEALIB_GPGSA_PREFIX " parse error: need 17 tokens, got %lu in '%s'", (long unsigned) fieldCount,
        s);
    goto err;
  }

  /* determine which fields are present and validate them */

  if (pack->sig) {
    if (!((pack->sig == 'A') //
        || (pack->sig == 'M'))) {
      nmeaContextError(NMEALIB_GPGSA_PREFIX " parse error: invalid selection mode '%c' in '%s'", pack->sig, s);
      goto err;
    }

    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_SIG);
  } else {
    pack->sig = '\0';
  }

  if (pack->fix != UINT_MAX) {
    if (!nmeaValidateFix(pack->fix, NMEALIB_GPGSA_PREFIX, s)) {
      goto err;
    }

    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_FIX);
  } else {
    pack->fix = NMEALIB_FIX_BAD;
  }

  qsort(pack->prn, NMEALIB_GPGSA_SATS_IN_SENTENCE, sizeof(unsigned int), nmeaQsortPRNCompact);
  if (!pack->prn[0]) {
    memset(pack->prn, 0, sizeof(pack->prn));
  } else {
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_SATINUSE);
  }

  if (!isnan(pack->pdop)) {
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_PDOP);
  } else {
    pack->pdop = 0.0;
  }

  if (!isnan(pack->hdop)) {
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_HDOP);
  } else {
    pack->hdop = 0.0;
  }

  if (!isnan(pack->vdop)) {
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_VDOP);
  } else {
    pack->vdop = 0.0;
  }

  return true;

err:
  memset(pack, 0, sizeof(*pack));
  pack->fix = NMEALIB_FIX_BAD;
  return false;
}

void nmeaGPGSAToInfo(const NmeaGPGSA *pack, NmeaInfo *info) {
  if (!pack //
      || !info) {
    return;
  }

  nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_SMASK);

  info->smask |= NMEALIB_SENTENCE_GPGSA;

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_SIG) //
      && (info->sig == NMEALIB_SIG_INVALID)) {
    if (pack->sig == 'M') {
      info->sig = NMEALIB_SIG_MANUAL;
    } else {
      info->sig = NMEALIB_SIG_FIX;
    }

    nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_SIG);
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_FIX)) {
    info->fix = pack->fix;
    nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_FIX);
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_SATINUSE)) {
    size_t packIndex = 0;
    size_t infoIndex = 0;

    info->satellites.inUseCount = 0;
    memset(&info->satellites.inUse, 0, sizeof(info->satellites.inUse[0]));

    for (packIndex = 0; (packIndex < NMEALIB_GPGSA_SATS_IN_SENTENCE) && (infoIndex < NMEALIB_MAX_SATELLITES);
        packIndex++) {
      unsigned int prn = pack->prn[packIndex];
      if (prn) {
        info->satellites.inUse[infoIndex++] = prn;
        info->satellites.inUseCount++;
      }
    }

    nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_SATINUSECOUNT | NMEALIB_PRESENT_SATINUSE);
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_PDOP)) {
    info->pdop = pack->pdop;
    nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_PDOP);
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_HDOP)) {
    info->hdop = pack->hdop;
    nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_HDOP);
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_VDOP)) {
    info->vdop = pack->vdop;
    nmeaInfoSetPresent(&info->present, NMEALIB_PRESENT_VDOP);
  }
}

void nmeaGPGSAFromInfo(const NmeaInfo *info, NmeaGPGSA *pack) {
  if (!pack //
      || !info) {
    return;
  }

  memset(pack, 0, sizeof(*pack));
  pack->fix = NMEALIB_FIX_BAD;

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SIG)) {
    if (info->sig == NMEALIB_SIG_MANUAL) {
      pack->sig = 'M';
    } else {
      pack->sig = 'A';
    }

    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_SIG);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_FIX)) {
    pack->fix = info->fix;
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_FIX);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_SATINUSE)) {
    size_t infoIndex = 0;
    size_t packIndex = 0;

    for (infoIndex = 0; (infoIndex < NMEALIB_MAX_SATELLITES) && (packIndex < NMEALIB_GPGSA_SATS_IN_SENTENCE);
        infoIndex++) {
      unsigned int prn = info->satellites.inUse[infoIndex];
      if (prn) {
        pack->prn[packIndex++] = prn;
      }
    }

    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_SATINUSE);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_PDOP)) {
    pack->pdop = info->pdop;
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_PDOP);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_HDOP)) {
    pack->hdop = info->hdop;
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_HDOP);
  }

  if (nmeaInfoIsPresentAll(info->present, NMEALIB_PRESENT_VDOP)) {
    pack->vdop = info->vdop;
    nmeaInfoSetPresent(&pack->present, NMEALIB_PRESENT_VDOP);
  }
}

size_t nmeaGPGSAGenerate(char *s, const size_t sz, const NmeaGPGSA *pack) {

#define dst       (&s[chars])
#define available ((sz <= (size_t) chars) ? 0 : (sz - (size_t) chars))

  int chars = 0;
  bool satInUse;
  size_t i;

  if (!s //
      || !pack) {
    return 0;
  }

  chars += snprintf(dst, available, "$" NMEALIB_GPGSA_PREFIX);

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_SIG) //
      && pack->sig) {
    chars += snprintf(dst, available, ",%c", pack->sig);
  } else {
    chars += snprintf(dst, available, ",");
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_FIX)) {
    chars += snprintf(dst, available, ",%d", pack->fix);
  } else {
    chars += snprintf(dst, available, ",");
  }

  satInUse = nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_SATINUSE);
  for (i = 0; i < NMEALIB_GPGSA_SATS_IN_SENTENCE; i++) {
    unsigned int prn = pack->prn[i];
    if (satInUse && prn) {
      chars += snprintf(dst, available, ",%d", prn);
    } else {
      chars += snprintf(dst, available, ",");
    }
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_PDOP)) {
    chars += snprintf(dst, available, ",%03.1f", pack->pdop);
  } else {
    chars += snprintf(dst, available, ",");
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_HDOP)) {
    chars += snprintf(dst, available, ",%03.1f", pack->hdop);
  } else {
    chars += snprintf(dst, available, ",");
  }

  if (nmeaInfoIsPresentAll(pack->present, NMEALIB_PRESENT_VDOP)) {
    chars += snprintf(dst, available, ",%03.1f", pack->vdop);
  } else {
    chars += snprintf(dst, available, ",");
  }

  /* checksum */
  chars += nmeaAppendChecksum(s, sz, (size_t) chars);

  return (size_t) chars;

#undef available
#undef dst

}
