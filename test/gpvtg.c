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

#include "testHelpers.h"

#include <nmealib/context.h>
#include <nmealib/gmath.h>
#include <nmealib/gpvtg.h>
#include <nmealib/info.h>
#include <nmealib/sentence.h>

#include <CUnit/Basic.h>
#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

int gpvtgSuiteSetup(void);

/*
 * Tests
 */

static void test_nmeaGPVTGParse(void) {
  const char * s = "some string";
  nmeaGPVTG packEmpty;
  nmeaGPVTG pack;
  bool r;

  memset(&packEmpty, 0, sizeof(packEmpty));
  memset(&pack, 0, sizeof(pack));

  /* invalid inputs */

  r = nmeaGPVTGParse(NULL, 1, &pack);
  validateParsePack(&pack, r, false, 0, 0, true);

  r = nmeaGPVTGParse(s, 0, &pack);
  validateParsePack(&pack, r, false, 0, 0, true);

  r = nmeaGPVTGParse(s, strlen(s), NULL);
  validateParsePack(&pack, r, false, 0, 0, true);

  /* invalid sentence / not enough fields */

  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  /* all fields empty */

  s = "$GPVTG,,,,,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, true, 1, 0, true);

  /* track */

  s = "$GPVTG,4.25,,,,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,4.25,q,,,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,4.25,t,,,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, true, 1, 0, false);
  CU_ASSERT_EQUAL(pack.present, TRACK);
  CU_ASSERT_DOUBLE_EQUAL(pack.track, 4.25, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.track_t, 'T');

  /* mtrack */

  s = "$GPVTG,,,4.25,,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,,,4.25,q,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,,,4.25,m,,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, true, 1, 0, false);
  CU_ASSERT_EQUAL(pack.present, MTRACK);
  CU_ASSERT_DOUBLE_EQUAL(pack.mtrack, 4.25, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.mtrack_m, 'M');

  /* speed knots */

  s = "$GPVTG,,,,,4.25,,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,,,,,4.25,q,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,,,,,4.25,n,,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, true, 1, 0, false);
  CU_ASSERT_EQUAL(pack.present, SPEED);
  CU_ASSERT_DOUBLE_EQUAL(pack.spn, 4.25, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.spn_n, 'N');
  CU_ASSERT_DOUBLE_EQUAL(pack.spk, (4.25 * NMEALIB_TUD_KNOTS), DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.spk_k, 'K');

  /* speed kph */

  s = "$GPVTG,,,,,,,4.25,*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,,,,,,,4.25,q*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, false, 1, 1, true);

  s = "$GPVTG,,,,,,,4.25,k*";
  r = nmeaGPVTGParse(s, strlen(s), &pack);
  validateParsePack(&pack, r, true, 1, 0, false);
  CU_ASSERT_EQUAL(pack.present, SPEED);
  CU_ASSERT_DOUBLE_EQUAL(pack.spk, 4.25, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.spk_k, 'K');
  CU_ASSERT_DOUBLE_EQUAL(pack.spn, (4.25 / NMEALIB_TUD_KNOTS), DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.spn_n, 'N');
}

static void test_nmeaGPVTGToInfo(void) {
  nmeaGPVTG pack;
  NmeaInfo infoEmpty;
  NmeaInfo info;

  memset(&pack, 0, sizeof(pack));
  memset(&infoEmpty, 0, sizeof(infoEmpty));
  memset(&info, 0, sizeof(info));

  /* invalid inputs */

  nmeaGPVTGToInfo(NULL, &info);
  validatePackToInfo(&info, 0, 0, true);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  nmeaGPVTGToInfo(&pack, NULL);
  validatePackToInfo(&info, 0, 0, true);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  /* empty */

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  /* track */

  pack.track = 42.75;
  nmeaInfoSetPresent(&pack.present, TRACK);

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK | TRACK);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  CU_ASSERT_DOUBLE_EQUAL(info.track, 42.75, DBL_EPSILON);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  /* mtrack */

  pack.mtrack = 42.75;
  nmeaInfoSetPresent(&pack.present, MTRACK);

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK | MTRACK);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  CU_ASSERT_DOUBLE_EQUAL(info.mtrack, 42.75, DBL_EPSILON);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  /* speed */

  pack.spn = 42.75;
  pack.spn_n = '\0';
  pack.spk = 10.0;
  pack.spk_k = '\0';
  nmeaInfoSetPresent(&pack.present, SPEED);

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK | SPEED);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  CU_ASSERT_DOUBLE_EQUAL(info.speed, (42.75 * NMEALIB_TUD_KNOTS), DBL_EPSILON);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  pack.spn = 42.75;
  pack.spn_n = 'N';
  pack.spk = 10.0;
  pack.spk_k = '\0';
  nmeaInfoSetPresent(&pack.present, SPEED);

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK | SPEED);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  CU_ASSERT_DOUBLE_EQUAL(info.speed, (42.75 * NMEALIB_TUD_KNOTS), DBL_EPSILON);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  pack.spn = 42.75;
  pack.spn_n = '\0';
  pack.spk = 10.0;
  pack.spk_k = 'K';
  nmeaInfoSetPresent(&pack.present, SPEED);

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK | SPEED);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  CU_ASSERT_DOUBLE_EQUAL(info.speed, 10.0, DBL_EPSILON);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));

  pack.spn = 42.75;
  pack.spn_n = 'N';
  pack.spk = 10.0;
  pack.spk_k = 'K';
  nmeaInfoSetPresent(&pack.present, SPEED);

  nmeaGPVTGToInfo(&pack, &info);
  validatePackToInfo(&info, 0, 0, false);
  CU_ASSERT_EQUAL(info.present, SMASK | SPEED);
  CU_ASSERT_EQUAL(info.smask, GPVTG);
  CU_ASSERT_DOUBLE_EQUAL(info.speed, 10.0, DBL_EPSILON);
  memset(&pack, 0, sizeof(pack));
  memset(&info, 0, sizeof(info));
}

static void test_nmeaGPVTGFromInfo(void) {
  NmeaInfo info;
  nmeaGPVTG packEmpty;
  nmeaGPVTG pack;

  memset(&info, 0, sizeof(info));
  memset(&packEmpty, 0, sizeof(packEmpty));
  memset(&pack, 0, sizeof(pack));

  /* invalid inputs */

  nmeaGPVTGFromInfo(NULL, &pack);
  validateInfoToPack(&pack, 0, 0, true);
  memset(&info, 0, sizeof(info));

  nmeaGPVTGFromInfo(&info, NULL);
  validateInfoToPack(&pack, 0, 0, true);
  memset(&info, 0, sizeof(info));

  /* empty */

  nmeaGPVTGFromInfo(&info, &pack);
  validateInfoToPack(&pack, 0, 0, true);
  memset(&info, 0, sizeof(info));

  /* track */

  info.track = 1232.5523;
  nmeaInfoSetPresent(&info.present, TRACK);

  nmeaGPVTGFromInfo(&info, &pack);
  validateInfoToPack(&pack, 0, 0, false);
  CU_ASSERT_EQUAL(pack.present, TRACK);
  CU_ASSERT_DOUBLE_EQUAL(pack.track, 1232.5523, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.track_t, 'T');
  memset(&info, 0, sizeof(info));

  /* mtrack */

  info.mtrack = 1232.5523;
  nmeaInfoSetPresent(&info.present, MTRACK);

  nmeaGPVTGFromInfo(&info, &pack);
  validateInfoToPack(&pack, 0, 0, false);
  CU_ASSERT_EQUAL(pack.present, MTRACK);
  CU_ASSERT_DOUBLE_EQUAL(pack.mtrack, 1232.5523, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.mtrack_m, 'M');
  memset(&info, 0, sizeof(info));

  /* speed */

  info.speed = 10.0;
  nmeaInfoSetPresent(&info.present, SPEED);

  nmeaGPVTGFromInfo(&info, &pack);
  validateInfoToPack(&pack, 0, 0, false);
  CU_ASSERT_EQUAL(pack.present, SPEED);
  CU_ASSERT_DOUBLE_EQUAL(pack.spn, (10.0 / NMEALIB_TUD_KNOTS), DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.spn_n, 'N');
  CU_ASSERT_DOUBLE_EQUAL(pack.spk, 10.0, DBL_EPSILON);
  CU_ASSERT_EQUAL(pack.spk_k, 'K');
  memset(&info, 0, sizeof(info));
}

static void test_nmeaGPVTGGenerate(void) {
  char buf[256];
  nmeaGPVTG pack;
  size_t r;

  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* invalid inputs */

  r = nmeaGPVTGGenerate(NULL, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 0);
  CU_ASSERT_EQUAL(*buf, '\0');
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  r = nmeaGPVTGGenerate(buf, sizeof(buf), NULL);
  CU_ASSERT_EQUAL(r, 0);
  CU_ASSERT_EQUAL(*buf, '\0');
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* empty with 0 length */

  r = nmeaGPVTGGenerate(buf, 0, &pack);
  CU_ASSERT_EQUAL(r, 19);
  CU_ASSERT_EQUAL(*buf, '\0');
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* empty */

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 19);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,,,,,,,,*52\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* track */

  pack.track = 42.6;
  pack.track_t = 'T';
  nmeaInfoSetPresent(&pack.present, TRACK);

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 24);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,42.6,T,,,,,,*18\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* mtrack */

  pack.mtrack = 42.6;
  pack.mtrack_m = 'M';
  nmeaInfoSetPresent(&pack.present, MTRACK);

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 24);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,,,42.6,M,,,,*01\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* speed knots */

  pack.spn = 42.6;
  pack.spn_n = '\0';
  nmeaInfoSetPresent(&pack.present, SPEED);

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 19);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,,,,,,,,*52\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  pack.spn = 42.6;
  pack.spn_n = 'N';
  nmeaInfoSetPresent(&pack.present, SPEED);

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 24);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,,,,,42.6,N,,*02\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  /* speed kph */

  pack.spk = 42.6;
  pack.spk_k = '\0';
  nmeaInfoSetPresent(&pack.present, SPEED);

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 19);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,,,,,,,,*52\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));

  pack.spk = 42.6;
  pack.spk_k = 'K';
  nmeaInfoSetPresent(&pack.present, SPEED);

  r = nmeaGPVTGGenerate(buf, sizeof(buf), &pack);
  CU_ASSERT_EQUAL(r, 24);
  CU_ASSERT_STRING_EQUAL(buf, "$GPVTG,,,,,,,42.6,K*07\r\n");
  memset(buf, 0, sizeof(buf));
  memset(&pack, 0, sizeof(pack));
}

/*
 * Setup
 */

static int suiteInit(void) {
  nmeaPrintFunction prev;

  prev = nmeaContextSetTraceFunction(traceFunction);
  if (prev) {
    return CUE_SINIT_FAILED;
  }

  prev = nmeaContextSetErrorFunction(errorFunction);
  if (prev) {
    return CUE_SINIT_FAILED;
  }

  mockContextReset();
  return CUE_SUCCESS;
}

static int suiteClean(void) {
  nmeaContextSetErrorFunction(NULL);
  nmeaContextSetTraceFunction(NULL);
  mockContextReset();
  return CUE_SUCCESS;
}

int gpvtgSuiteSetup(void) {
  CU_pSuite pSuite = CU_add_suite("gpvtg", suiteInit, suiteClean);
  if (!pSuite) {
    return CU_get_error();
  }

  if ( //
      (!CU_add_test(pSuite, "nmeaGPVTGParse", test_nmeaGPVTGParse)) //
      || (!CU_add_test(pSuite, "nmeaGPVTGToInfo", test_nmeaGPVTGToInfo)) //
      || (!CU_add_test(pSuite, "nmeaGPVTGFromInfo", test_nmeaGPVTGFromInfo)) //
      || (!CU_add_test(pSuite, "nmeaGPVTGGenerate", test_nmeaGPVTGGenerate)) //
      ) {
    return CU_get_error();
  }

  return CUE_SUCCESS;
}
