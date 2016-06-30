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

#ifndef __NMEALIB_GENERATOR_H__
#define __NMEALIB_GENERATOR_H__

#include <nmealib/info.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* forward declaration */
struct _nmeaGENERATOR;

/**
 * Generator type enum
 */
enum nmeaGENTYPE {
  NMEALIB_GEN_NOISE = 0,
  NMEALIB_GEN_STATIC,
  NMEALIB_GEN_ROTATE,
  NMEALIB_GEN_SAT_STATIC,
  NMEALIB_GEN_SAT_ROTATE,
  NMEALIB_GEN_POS_RANDMOVE,
  NMEALIB_GEN_LAST
};

struct _nmeaGENERATOR * nmea_create_generator(const int type, NmeaInfo *info);
size_t nmea_generate_from(char *buff, size_t buff_sz, NmeaInfo *info, struct _nmeaGENERATOR *gen, int generate_mask);

/**
 * Generator initialiser function definition.
 *
 * @param gen a pointer to the generator
 * @param info a pointer to an nmeaINFO structure to use during generation
 * @return true on success, false otherwise
 */
typedef bool (*nmeaNMEA_GEN_INIT)(struct _nmeaGENERATOR *gen, NmeaInfo *info);

/**
 * Generator loop function definition.
 *
 * @param gen a pointer to the generator
 * @param info a pointer to an nmeaINFO structure to use during generation
 * @return true on success, false otherwise
 */
typedef bool (*nmeaNMEA_GEN_LOOP)(struct _nmeaGENERATOR *gen, NmeaInfo *info);

/**
 * Generator reset function definition.
 *
 * @param gen a pointer to the generator
 * @param info a pointer to an nmeaINFO structure to use during generation
 * @return true on success, false otherwise
 */
typedef bool (*nmeaNMEA_GEN_RESET)(struct _nmeaGENERATOR *gen, NmeaInfo *info);

/**
 * Generator destroy function definition.
 *
 * @param gen a pointer to the generator
 * @return true on success, false otherwise
 */
typedef bool (*nmeaNMEA_GEN_DESTROY)(struct _nmeaGENERATOR *gen);

/**
 * Generator structure
 */
typedef struct _nmeaGENERATOR {
    void *gen_data; /**< generator data */
    nmeaNMEA_GEN_INIT init_call; /**< initialiser function */
    nmeaNMEA_GEN_LOOP loop_call; /**< loop function */
    nmeaNMEA_GEN_RESET reset_call; /**< reset function */
    nmeaNMEA_GEN_DESTROY destroy_call; /**< destroy function */
    struct _nmeaGENERATOR *next; /**< the next generator */
} nmeaGENERATOR;

bool nmea_gen_init(nmeaGENERATOR *gen, NmeaInfo *info);
bool nmea_gen_loop(nmeaGENERATOR *gen, NmeaInfo *info);
bool nmea_gen_reset(nmeaGENERATOR *gen, NmeaInfo *info);
void nmea_gen_destroy(nmeaGENERATOR *gen);
void nmea_gen_add(nmeaGENERATOR *to, nmeaGENERATOR *gen);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NMEALIB_GENERATOR_H__ */
